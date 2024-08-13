#include "driver.h"
#include "gyro_xyz.h"
#include "message_coder.h"
#include <cstdint>
#include <stdexcept>
#include <unistd.h>
#include <vector>

SensorDriver::SensorDriver(IOInterface &interface)
    : command_message_coder_(DELIM), response_message_coder_(DELIM),
      data_response_message_coder_(DELIM), io_interface_(interface){};

SensorDriver::SensorDriver(MessageCoder<CommandRaw_t> command_coder,
                           MessageCoder<ResponseRaw_t> response_coder,
                           MessageCoder<DataResponseRaw_t> data_response_coder,
                           IOInterface &interface)
    : command_message_coder_(command_coder),
      response_message_coder_(response_coder),
      data_response_message_coder_(data_response_coder),
      io_interface_(interface){};

void SensorDriver::init() { io_interface_.init(); }
void SensorDriver::shutdown() { io_interface_.shutdown(); }

bool SensorDriver::isAlive() {
  // each of these functions behaves pretty similarly:
  // 1. send command
  // 2. look for, and consume, a response
  // With a proper decoder this would be a little less dubious. e.g. what if
  // between step 1.) and step 2.) we get sent an automated data message?
  // Currently these two steps have to happen back-to-back to keep from losing
  // sync.

  sendCommand(VERSION_GET_REG);
  auto data = receiveResponse(VERSION_GET_REG);
  if (data.data != 0) {
    return true;
  }
  return false;
};

uint8_t SensorDriver::getVersion() {
  sendCommand(VERSION_GET_REG);
  auto data = receiveResponse(VERSION_GET_REG);
  return data.data;
};

uint8_t SensorDriver::setMode(uint8_t mode) {
  // io_interface_.flush();
  sendCommand(MODE_SET_REG, mode);
  auto data = receiveResponse(MODE_SET_REG);
  return data.data;
};

// gets the mode the device is currently in
uint8_t SensorDriver::getMode() {
  sendCommand(MODE_GET_REG);
  auto data = receiveResponse(MODE_GET_REG);
  return data.data;
};

// get some number of rates
std::vector<DataResponseRaw_t> SensorDriver::getRates() {
  sendCommand(DATA_GET_REG);
  auto data = receiveDataResponse();
  return data;
};

void SensorDriver::sendCommand(uint8_t cmd) {
  sendCommand(cmd, 0);
  return;
}

void SensorDriver::sendCommand(uint8_t cmd, uint8_t data) {
  auto cmdRaw = CommandRaw_t{.addr = cmd, .data = data, .delim = DELIM};
  io_interface_.send(command_message_coder_.frame(cmdRaw));
};

ResponseRaw_t SensorDriver::receiveResponse(uint8_t reg) {
  // this function and the next are hacked together in such a way as to handle
  // the issue mentioned above (having an automated data messages sneak in
  // before the desired response).
  //
  // The mitigation for this is to look for the response type we're interested
  // in (ResponseRaw_t here, and DataResponseRaw_t in the following fcn) - then
  // deframe that, and discard everything else in the input buffer. There are
  // better ways that don't drop data, but I was thinking they'd be more error
  // prone.
  //
  // There is also a hard-coded retry limit, as a very crude response timeout

  size_t bytes_avail;
  std::vector<ResponseRaw_t> resps;
  size_t count = 0;

  while (resps.empty() && count < RESPONSE_RECEIVE_RETRY_LIMIT) {
    // Set the number of bytes to attempt retrieving to the larger of a.) how
    // many are in the buffer, or b.) the minimum frame size for this message
    // type
    //
    // If b.), then the call to `retrieve` will block.
    bytes_avail = std::max((size_t)io_interface_.availableBytes(),
                           response_message_coder_.getFrameLength());

    auto data = io_interface_.receive(bytes_avail);

    // append received bytes to an internal buffer, which is used in case we get
    // a partial message
    rx_.insert(rx_.end(), data.begin(), data.end());

    // Parse stream of bytes into a set of de-framed messages.
    // Note: `deFrame` consumes `rx_` in the process.
    resps = response_message_coder_.deFrame(rx_);

    count++;
  }

  // Crude hack here: even if we got multiple valid responses, just return the
  // first one. In the real world we'd have better message ids, and we'd look
  // for the one that matches the command we sent.
  for (auto &resp : resps) {
    if (resp.addr == reg) {
      return resp;
    }
  }

  throw std::runtime_error("Could not find message for reg in responses.");
};

std::vector<DataResponseRaw_t> SensorDriver::receiveDataResponse() {
  // Similar function as above, but instead for DataResponseRaw_t's
  // This is used for both a request-response in `getRates()` as well as the
  // primary method to get multiple rates when the sensor is in auto mode.

  size_t bytes_avail;
  std::vector<DataResponseRaw_t> resps;
  size_t count = 0;

  while (resps.empty() && count < RESPONSE_RECEIVE_RETRY_LIMIT) {
    bytes_avail = std::max((size_t)io_interface_.availableBytes(),
                           data_response_message_coder_.getFrameLength());

    auto data = io_interface_.receive(bytes_avail);

    rx_.insert(rx_.end(), data.begin(), data.end());

    resps = data_response_message_coder_.deFrame(rx_);

    count++;
  }

  if (resps.size() < 1) {
    throw std::runtime_error("Didn't receive any data responses!");
  }

  return resps;
};
