#include <Arduino.h>
#include <string>

#include <ReliableConnectionUsbCdc.h>

ReliableConnectionUsbCdc usb = ReliableConnectionUsbCdc();

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

  Serial.println("setup() complete.");
  Serial.write("\033[2J\033[H"); // Clear screen and send cursor to home
}

void loop()
{
  std::vector<char> output = usb->read();
  if(!output.empty())
  {
    usb.write(output);
  }
}
