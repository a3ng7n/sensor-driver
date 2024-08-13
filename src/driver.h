#pragma once
#include "io_interface.h"
#include "message_coder.h"
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

const size_t RESPONSE_RECEIVE_RETRY_LIMIT = 5;

// top-level driver class
class SensorDriver {
public:
  SensorDriver(IOInterface &interface);
  SensorDriver(MessageCoder<CommandRaw_t> command_coder,
               MessageCoder<ResponseRaw_t> response_coder,
               MessageCoder<DataResponseRaw_t> data_response_coder,
               IOInterface &interface);

  ~SensorDriver() { shutdown(); };
  void init();
  void shutdown();

  // check whether the device is responsive
  // i.e. sends a NOOP
  bool isAlive();

  // gets the device's version information
  uint8_t getVersion();

  // NOT IMPLEMENTED: generic register writing
  bool writeRegister(uint8_t addr, uint8_t value);

  // NOT IMPLEMENTED: generic register reading
  uint8_t sendRegisterRead(uint8_t addr);

  // sets the device to be in `mode`
  uint8_t setMode(uint8_t mode);

  // gets the mode the device is currently in
  uint8_t getMode();

  // get some number of rates
  std::vector<DataResponseRaw_t> getRates();

  // generalized sending of commands
  void sendCommand(uint8_t cmd, uint8_t data);
  void sendCommand(uint8_t cmd);

  // generalized receiving of responses
  ResponseRaw_t receiveResponse(uint8_t reg);

  // generalized receiving of data
  std::vector<DataResponseRaw_t> receiveDataResponse();

private:
  std::vector<uint8_t> rx_;

  MessageCoder<CommandRaw_t> command_message_coder_;
  MessageCoder<ResponseRaw_t> response_message_coder_;
  MessageCoder<DataResponseRaw_t> data_response_message_coder_;
  IOInterface &io_interface_;
};
