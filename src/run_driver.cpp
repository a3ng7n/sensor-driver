#include "driver.h"
#include "gyro_xyz.h"
#include "io_interface.h"
#include "message_coder.h"
#include <ios>
#include <iostream>
#include <ostream>
#include <string>
#include <unistd.h>

int main() {
  std::string myport("/tmp/ttyDRIVER");
  IOInterface myio = IOInterface(myport, 38400);
  SensorDriver mydriver = SensorDriver(myio);

  mydriver.init();

  std::cout << "getMode: " << (int)mydriver.getMode() << std::endl;
  std::cout << "getVersion: " << std::hex << (int)mydriver.getVersion()
            << std::dec << std::endl;
  std::cout << "isAlive: " << mydriver.isAlive() << std::endl;

  auto rates = mydriver.getRates();
  for (auto &data : rates) {
    printMessage(data);
  }

  mydriver.setMode(MODE_ARG_CONFIG);
  std::cout << "getMode: " << (int)mydriver.getMode() << std::endl;

  mydriver.setMode(MODE_ARG_MANUAL);
  std::cout << "getMode: " << (int)mydriver.getMode() << std::endl;

  mydriver.setMode(MODE_ARG_AUTO);
  std::cout << "getMode: " << (int)mydriver.getMode() << std::endl;

  for (int i = 0; i < 10; i++) {
    auto rates = mydriver.receiveDataResponse();
    for (auto &data : rates) {
      printMessage(data);
    }
  }

  mydriver.setMode(MODE_ARG_MANUAL);
  std::cout << "getMode: " << (int)mydriver.getMode() << std::endl;

  mydriver.shutdown();
  return 0;
}
