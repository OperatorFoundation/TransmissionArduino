//
// Created by Dr. Brandon Wiley on 11/6/2025.
//

#include "ReliableConnectionSerial1.h"

#include <Arduino.h>

ReliableConnectionSerial1::ReliableConnectionSerial1()
{
}

int ReliableConnectionSerial1::tryReadOne()
{
  return Serial1.read();
}

char ReliableConnectionSerial1::readOne()
{
  // Wait for serial port to be ready
  while(!Serial1 || !Serial1.available())
  {
    delay(0.01);
  }

  int b = -1;
  while(b == -1)
  {
    b = Serial1.read();
  }

  return (byte)b;
}

std::vector<char> ReliableConnectionSerial1::read()
{
  int available = Serial1.available();
  if (available == 0)
  {
    return std::vector<char>();
  }

  std::vector<char> bs = std::vector<char>(available);

  int bytesRead = Serial1.readBytes(bs.data(), available);
  bs.resize(bytesRead);

  return bs;
}

std::vector<char> ReliableConnectionSerial1::read(int size)
{
  std::vector<char> r = std::vector<char>();

  while(static_cast<int>(r.size()) < size)
  {
    byte b = readOne();
    r.push_back(b);
  }

  return r;
}

void ReliableConnectionSerial1::write(std::vector<char> bs)
{
  Serial1.write(bs.data(), bs.size()); // Send each byte
}

bool ReliableConnectionSerial1::availableForReading()
{
  int bytesAvailable = Serial1.available();

  return bytesAvailable > 0;
}
