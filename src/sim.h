#pragma once
#include "io_interface.h"
#include "message_coder.h"
#include <cstdint>
#include <deque>

// top-level driver class
class SensorSim {
public:
  SensorSim(IOInterface &interface);
  SensorSim(MessageCoder<CommandRaw_t> command_coder,
            MessageCoder<ResponseRaw_t> response_coder,
            MessageCoder<DataResponseRaw_t> data_respons_coder,
            IOInterface &interface);
  ~SensorSim() { shutdown(); };

  void init();
  void run();
  void shutdown();

private:
  // retrieve bytes from io, and add to command queue
  void processInput();

  // process commands on the command queue
  // placeholder for logic like command input rate-limiting
  void processCommands();

  // process responses on the response queue
  void processResponses();

  // dispatch an individual command
  void dispatchCommand(CommandRaw_t &cmd);

  // issue an invididual response
  void issueResponse(ResponseRaw_t &rsp);
  void issueResponse(DataResponseRaw_t &rsp);

  // handle any per-mode logic
  void processMode();

  // sets the device to be in `mode`
  void setMode(uint8_t mode);

  void getTruth();

  // current rate values (populated by getTruth)
  float x_rate_;
  float y_rate_;
  float z_rate_;

  // the current mode
  uint8_t mode_;

  // the current sample count - increments for ever data response
  uint16_t counter_;

  // fifo queues for commands and respones
  std::deque<CommandRaw_t> commands_;
  std::vector<uint8_t> rx_;
  std::deque<ResponseRaw_t> responses_;
  std::deque<DataResponseRaw_t> data_responses_;

  // message decode/encoders
  MessageCoder<CommandRaw_t> command_message_coder_;
  MessageCoder<ResponseRaw_t> response_message_coder_;
  MessageCoder<DataResponseRaw_t> data_response_message_coder_;

  // uart io
  IOInterface &io_interface_;
};
