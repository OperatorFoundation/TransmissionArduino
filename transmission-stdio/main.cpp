// main.cpp
#include "TransmissionMain.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
    std::cerr << "Example: " << argv[0] << " 192.168.1.100 5900" << std::endl;
    return 1;
  }

  std::string host = argv[1];
  uint16_t port = std::stoi(argv[2]);

  TransmissionMain vnc_client(host, port);

  if (!vnc_client.connect()) {
    return 1;
  }

  // Main loop
  while (true) {
    vnc_client.loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return 0;
}