#include "sim.h"
#include "gyro_xyz.h"
#include "io_interface.h"
#include "message_coder.h"
#include <cmath>
#include <csignal>
#include <cstdint>
#include <ios>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <vector>

// running this as a non-blocking loop
// this could be entirely event driven, but that's a little on the complicated
// side of things
uint8_t rate_mili = 100;
uint8_t rate_micro = rate_mili * 1000;

SensorSim::SensorSim(IOInterface &interface)
    : mode_(MODE_ARG_MANUAL), counter_(0), command_message_coder_(DELIM),
      response_message_coder_(DELIM), data_response_message_coder_(DELIM),
      io_interface_(interface){};

SensorSim::SensorSim(MessageCoder<CommandRaw_t> command_coder,
                     MessageCoder<ResponseRaw_t> response_coder,
                     MessageCoder<DataResponseRaw_t> data_response_coder,
                     IOInterface &interface)
    : mode_(MODE_ARG_MANUAL), counter_(0),
      command_message_coder_(command_coder),
      response_message_coder_(response_coder),
      data_response_message_coder_(data_response_coder),
      io_interface_(interface){};

void SensorSim::init() { io_interface_.init(); }
void SensorSim::shutdown() { io_interface_.shutdown(); }

void SensorSim::processInput() {
  uint8_t cmd_frame_len;
  int bytes_avail;

  // no data available - do nothing and move on
  if ((bytes_avail = io_interface_.availableBytes()) < 1) {
    return;
  }

  // append received bytes to internal byte buffer
  std::vector<uint8_t> rxBytes = io_interface_.receive(bytes_avail);
  rx_.insert(rx_.end(), rxBytes.begin(), rxBytes.end());

  // no enough data available - do nothing and move on
  if (rx_.size() < (cmd_frame_len = command_message_coder_.getFrameLength())) {
    return;
  }

  std::vector<CommandRaw_t> cmds = command_message_coder_.deFrame(rx_);

  for (auto &cmd : cmds) {
    printMessage(cmd);
    commands_.insert(commands_.begin(), cmd);
  }
}

void SensorSim::processCommands() {
  while (commands_.size() > 0) {
    auto &cmd = *commands_.begin();
    dispatchCommand(cmd);
    commands_.pop_front();
  }
}

void SensorSim::dispatchCommand(CommandRaw_t &cmd) {
  switch (cmd.addr) {
  case DATA_GET_REG:
    data_responses_.push_back(DataResponseRaw_t{.addr = DATA_GET_REG,
                                                .count = counter_,
                                                .x_rate = x_rate_,
                                                .y_rate = y_rate_,
                                                .z_rate = z_rate_,
                                                .delim = DELIM});
    counter_++;
    return;
  case VERSION_GET_REG:
    responses_.push_back(
        ResponseRaw_t{.addr = VERSION_GET_REG, .data = 0x23, .delim = DELIM});
    return;
  case MODE_SET_REG:
    setMode(cmd.data);
    responses_.push_back(
        ResponseRaw_t{.addr = MODE_SET_REG, .data = mode_, .delim = DELIM});
    return;
  case MODE_GET_REG:
    responses_.push_back(
        ResponseRaw_t{.addr = MODE_GET_REG, .data = mode_, .delim = DELIM});
    return;
  default:
    std::cout << "command not found: " << std::hex << (int)cmd.addr
              << std::endl;
    return;
  }
}

void SensorSim::processResponses() {
  while (responses_.size() > 0) {
    auto &resp = *responses_.begin();
    printMessage(resp);
    issueResponse(resp);
    responses_.pop_front();
  }
  while (data_responses_.size() > 0) {
    auto &resp = *data_responses_.begin();
    printMessage(resp);
    issueResponse(resp);
    data_responses_.pop_front();
  }
}

void SensorSim::issueResponse(ResponseRaw_t &resp) {
  io_interface_.send(response_message_coder_.frame(resp));
  return;
}

void SensorSim::issueResponse(DataResponseRaw_t &resp) {
  io_interface_.send(data_response_message_coder_.frame(resp));
  return;
}

void SensorSim::processMode() {
  switch (mode_) {
  case MODE_ARG_AUTO:
    data_responses_.push_back(DataResponseRaw_t{.addr = DATA_GET_REG,
                                                .count = counter_,
                                                .x_rate = x_rate_,
                                                .y_rate = y_rate_,
                                                .z_rate = z_rate_,
                                                .delim = DELIM});
    counter_++;
    return;
  case MODE_ARG_MANUAL:
    return;
  case MODE_ARG_CONFIG:
    return;
  default:
    std::cout << "unrecognizd mode: " << std::hex << (int)mode_ << std::endl;
    return;
  }
}

void SensorSim::getTruth() {
  // this is where we'd customize how the sim gets the "truth" data
  // e.g. retrieve it from a file of timestamps/uptimes + rates
  // or retrieve it from a udp socket.
  // In this case we're just setting the data to a sin function.

  float time = (counter_ * rate_mili) / 1000.0; // time in seconds
  float w = 0.5 * 2 * M_PI;                     // 0.05 Hz oscillation
  x_rate_ = 0.75 * std::sin(time * w);
  y_rate_ = 0.75 * std::sin(time * w + 0.66 * M_PI);
  z_rate_ = 0.75 * std::sin(time * w + 1.22 * M_PI);
  return;
};

void SensorSim::run() {
  while (1) {
    getTruth();

    processMode();

    processInput();

    processCommands();

    processResponses();

    usleep(rate_micro);
  }
}

void SensorSim::setMode(uint8_t mode) { mode_ = mode; }

int main() {
  std::string myport("/tmp/ttySIM");
  IOInterface myio = IOInterface(myport, 38400);
  SensorSim mysim = SensorSim(myio);

  mysim.init();
  mysim.run();
  mysim.shutdown();

  return 0;
}
