#include "io_interface.h"
#include <cstdint>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

IOInterface::IOInterface(const std::string &port, int baud_rate)
    : IOInterface::IOInterface(port, baud_rate, O_RDWR | O_NOCTTY | O_SYNC){};

IOInterface::IOInterface(const std::string &port, int baud_rate, int flags)
    : port_(port), baud_rate_(baud_rate), flags_(flags){};

void IOInterface::init() {
  fd_ = open(port_.c_str(), flags_);

  if (fd_ < 0) {
    throw std::runtime_error("Failed to open port " + port_);
  }

  configurePort(baud_rate_);
  flush();
}

void IOInterface::shutdown() { close(fd_); }

IOInterface::~IOInterface() { shutdown(); }

void IOInterface::send(const std::vector<uint8_t> &message) {
  // make sure it's actually an ssize_t
  ssize_t message_size = message.size();

  // write everything, if it didn't work throw
  if (write(fd_, message.data(), message.size()) != message_size) {
    throw std::runtime_error("Failed to send message");
  }
}

std::vector<uint8_t> IOInterface::receive(size_t size) {
  ssize_t bytes_read;

  // create a buffer to hold our bytes
  std::vector<uint8_t> buffer(size);

  // read up to `size`, if it didn't work throw
  if ((bytes_read = read(fd_, buffer.data(), size)) < 0) {
    throw std::runtime_error("Failed to receive message");
  }

  // don't forget to resize to what was actually read
  buffer.resize(bytes_read);
  return buffer;
}

int IOInterface::availableBytes() {
  int bytes_available = 0;
  if (ioctl(fd_, FIONREAD, &bytes_available) < 0) {
    throw std::runtime_error("Failed to get available bytes");
  }
  return bytes_available;
}

void IOInterface::flush() {
  if (tcflush(fd_, TCIFLUSH) != 0) {
    throw std::runtime_error("Failed to clear input buffer");
  }
}

void IOInterface::configurePort(int baud_rate) {
  struct termios tty;

  // read in state of fd into a `termios` struct
  if (tcgetattr(fd_, &tty) != 0) {
    throw std::runtime_error("Failed to get UART attributes");
  }

  // set input and output baud rate the same
  cfsetospeed(&tty, baud_rate);
  cfsetispeed(&tty, baud_rate);

  // set all the flags
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;

  // control characters
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 5;

  // input modes
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);

  // control modes
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    throw std::runtime_error("Failed to set UART attributes");
  }
}
