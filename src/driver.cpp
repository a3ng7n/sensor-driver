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
  size_t bytes_avail;
  std::vector<ResponseRaw_t> resps;
  size_t count = 0;

  while (resps.empty() && count < RESPONSE_RECEIVE_RETRY_LIMIT) {
    bytes_avail =
        std::max((size_t)io_interface_.availableBytes(), sizeof(ResponseRaw_t));

    auto data = io_interface_.receive(bytes_avail);

    rx_.insert(rx_.end(), data.begin(), data.end());

    resps = response_message_coder_.deFrame(rx_);

    count++;
  }

  for (auto &resp : resps) {
    if (resp.addr == reg) {
      return resp;
    }
  }

  throw std::runtime_error("Could not find message for reg in responses.");
};

std::vector<DataResponseRaw_t> SensorDriver::receiveDataResponse() {
  size_t bytes_avail;
  std::vector<DataResponseRaw_t> resps;
  size_t count = 0;

  while (resps.empty() && count < RESPONSE_RECEIVE_RETRY_LIMIT) {
    bytes_avail = std::max((size_t)io_interface_.availableBytes(),
                           sizeof(DataResponseRaw_t));

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
