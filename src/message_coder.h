#pragma once
#include "io_interface.h"
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

// handles any special data formatting / framing / de-framing
// e.g. SLIP or something more specialized
// in this case it just serializes structs into byte vectors
// with a constant delimiter
template <typename T> class MessageCoder {
public:
  MessageCoder(uint8_t delimiter)
      : delim_(delimiter), frame_length_(sizeof(T)){};

  MessageCoder(uint8_t delimiter, uint8_t frame_length)
      : delim_(delimiter), frame_length_(frame_length){};

  // Returns a frame created from `payload`
  std::vector<uint8_t> frame(T &payload);

  // Returns a vector of payloads extracted from `data`
  std::vector<T> deFrame(std::vector<uint8_t> &data);

  auto getFrameLength() { return frame_length_; };

private:
  uint8_t delim_;
  size_t frame_length_;
};

struct CommandRaw {
  uint8_t addr;
  uint8_t data;
  uint8_t delim;
} typedef CommandRaw_t;

struct ResponseRaw {
  uint8_t addr;
  uint8_t data;
  uint8_t delim;
} typedef ResponseRaw_t;

#pragma pack(push, 1)
struct DataResponseRaw {
  uint8_t addr;
  uint16_t count;
  float x_rate;
  float y_rate;
  float z_rate;
  uint8_t delim;
} typedef DataResponseRaw_t;
#pragma pack(pop)

template <typename T> void printMessage(T &data);
