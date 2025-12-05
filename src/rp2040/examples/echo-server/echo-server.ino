#include <Arduino.h>
#include <string>

#include <ReliableConnectionUsbCdc.h>
#include <ReliableConnectionSerial1.h>

ReliableConnectionUsbCdc usb = ReliableConnectionUsbCdc();
ReliableConnectionSerial1* serial1 = ReliableConnectionSerial1::getInstance();

void setup()
{
  // Wait for USB serial with timeout
  unsigned long start = millis();
  while(!Serial && millis() - start < 5000)  // 5 second timeout
  {
    delay(1);
  }

  Serial.begin(921600);
  Serial.println("Hello, Operator.");
  delay(2000); // Wait for USB to be initialized

  serial1->enableXonXoff();
  serial1->begin();

  Serial.println("setup() complete.");
  Serial.write("\033[2J\033[H"); // Clear screen and send cursor to home
}

void loop()
{
  std::vector<char> output = serial1->read();
  if(!output.empty())
  {
    usb.write(output);
  }

  std::vector<char> input = usb.read();
  if(!input.empty())
  {
    serial1->write(input);
  }
}
