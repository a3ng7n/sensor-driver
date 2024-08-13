#include "message_coder.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

template <>
std::vector<uint8_t> MessageCoder<CommandRaw_t>::frame(CommandRaw_t &payload) {
  std::vector<uint8_t> output = {payload.addr, payload.data, payload.delim};
  return output;
}

template <>
std::vector<uint8_t>
MessageCoder<ResponseRaw_t>::frame(ResponseRaw_t &payload) {
  std::vector<uint8_t> output = {payload.addr, payload.data, payload.delim};
  return output;
}

template <typename T>
std::vector<T> MessageCoder<T>::deFrame(std::vector<uint8_t> &data) {
  if (data.size() < frame_length_) {
    throw std::runtime_error("not enough data");
  }

  std::vector<T> resps;
  std::vector<uint8_t> buffer;

  for (auto &item : data) {

    buffer.push_back(item);
    if (item == delim_) {

      if (buffer.size() == frame_length_) {
        auto data_resp = reinterpret_cast<T *>(&buffer[0]);
        resps.push_back(data_resp[0]);
        buffer.clear();
      }
    }
  }

  data.clear();

  return resps;
}

template std::vector<ResponseRaw_t>
MessageCoder<ResponseRaw_t>::deFrame(std::vector<uint8_t> &data);
template std::vector<DataResponseRaw_t>
MessageCoder<DataResponseRaw_t>::deFrame(std::vector<uint8_t> &data);
template std::vector<CommandRaw_t>
MessageCoder<CommandRaw_t>::deFrame(std::vector<uint8_t> &data);

template <>
std::vector<uint8_t>
MessageCoder<DataResponseRaw_t>::frame(DataResponseRaw_t &payload) {
  // I don't know that this works if structs are unpacked - alternative to be
  // safe is just manually casting struct elements
  auto ptr = reinterpret_cast<uint8_t *>(&payload);
  std::vector<uint8_t> output(ptr, ptr + sizeof(payload));
  return output;
}

template <> void printMessage(CommandRaw_t &data) {
  std::cout << "command: addr: ";
  std::cout << std::hex << (int)data.addr << std::dec;
  std::cout << ", data: ";
  std::cout << std::hex << (int)data.data << std::dec;
  std::cout << ", delim: ";
  std::cout << std::hex << (int)data.delim << std::dec << std::endl;
}

template <> void printMessage(ResponseRaw_t &data) {
  std::cout << "response: addr: ";
  std::cout << std::hex << (int)data.addr << std::dec;
  std::cout << ", data: ";
  std::cout << std::hex << (int)data.data << std::dec;
  std::cout << ", delim: ";
  std::cout << std::hex << (int)data.delim << std::dec << std::endl;
}

template <> void printMessage(DataResponseRaw_t &data) {
  std::cout << "data: addr: ";
  std::cout << std::hex << (int)data.addr << std::dec;
  std::cout << ", count: ";
  std::cout << data.count << std::dec;
  std::cout << ", x_rate: ";
  std::cout << data.x_rate << std::dec;
  std::cout << ", y_rate: ";
  std::cout << data.y_rate << std::dec;
  std::cout << ", z_rate: ";
  std::cout << data.z_rate << std::dec;
  std::cout << ", delim: ";
  std::cout << std::hex << (int)data.delim << std::dec << std::endl;
}
