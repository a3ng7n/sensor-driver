#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// should probably namespace all of this, but not doing that for simplicity

// handles instantiating and sending data to hardware-level interface
// in this case it's a UART - but there might be another one for SPI, etc.
class IOInterface {
public:
  // Dumb initializers - separate from `init()`
  // so we can chose when to call `init()` if needed
  IOInterface(const std::string &port, int baud_rate);
  IOInterface(const std::string &port, int baud_rate, int flags);

  // Destructor. Runs shutdown().
  ~IOInterface();

  // Attempt to initialize the port
  void init();

  // Attempt to shutdown the port
  void shutdown();

  // Send `message`
  void send(const std::vector<uint8_t> &message);

  // Read `size` bytes
  std::vector<uint8_t> receive(size_t size);

  // Get number of bytes available on buffer
  int availableBytes();

  // Flush the input
  void flush();

private:
  std::string port_;
  int baud_rate_;
  int flags_;

  int fd_;

  // Configure the port
  void configurePort(int baud_rate);
};
