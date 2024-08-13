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
  // this is somewhat similar to the SensorDriver::receiveX() functions
  // although it's a little easier since we only need to receive one type of
  // data (CommandRaw_t's).
  // And yeah, this is all pretty hacky, and likely not
  // performant - I'm admittedly pretty weak in this area.

  uint8_t cmd_frame_len;
  int bytes_avail;

  // no data available - do nothing and move on
  if ((bytes_avail = io_interface_.availableBytes()) < 1) {
    return;
  }

  // append received bytes to an internal byte buffer, which is used
  // in case we get a partial message on a cycle
  std::vector<uint8_t> rxBytes = io_interface_.receive(bytes_avail);
  rx_.insert(rx_.end(), rxBytes.begin(), rxBytes.end());

  // no *enough* data available - do nothing and move on
  if (rx_.size() < (cmd_frame_len = command_message_coder_.getFrameLength())) {
    return;
  }

  // Parse stream of bytes into a set of commands, if any are found
  // Note: `deFrame` consumes `rx_` in the process - there's likely a better way
  // but I just wanted to keep the edge-case logic to a minimum for parsing.
  std::vector<CommandRaw_t> cmds = command_message_coder_.deFrame(rx_);

  // Add each command to the internal commands queue - I think I could do this
  // with an in-place `insert`, but I ran out of time and wanted to do it this
  // way for readability
  for (auto &cmd : cmds) {
    printMessage(cmd);
    commands_.insert(commands_.begin(), cmd);
  }
}

void SensorSim::processCommands() {
  // using a deque here just for readability - dispatch the commands in the
  // order they came in
  while (commands_.size() > 0) {
    auto &cmd = *commands_.begin();
    dispatchCommand(cmd);
    commands_.pop_front();
  }
}

void SensorSim::dispatchCommand(CommandRaw_t &cmd) {
  // big switch statement to handle all of the command types
  // The commands really should be an enum, but I got a bit strapped for time
  // and figured in place of a refactor I could just explain myself.

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
    // we could error here, but I figured the real imu wouldn't crash in this
    // case
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
  // intent is to run any *automatic* things per-mode.
  // Again, just a hack to show the behavior. With more time the proper way
  // to handle actions within modes is to emulate the state/transition diagram
  // for the given device.

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
  // every "cycle" loop through the standard set of tasks
  while (1) {
    getTruth();

    processMode();

    // retrieve bytes from uart, and deframe them as commands
    // this should be non-blocking, intentionally
    processInput();

    processCommands();

    processResponses();

    // the time-cycling of this is very poorly done - totally understood that
    // this loop will take longer than `rate_micro`, and it won't be consistent.
    // The proper way to do this is one of:
    // a.) rearchitect the input to be event-driven - which I think makes the
    // state/mode logic more complicated, or;
    // b.) run this asyncronously, or calculate some `wait_until` here, or;
    // c.) run this loop off of an interrupt
    //
    // there are probably others, and I'm far from an expert on all the choices
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
