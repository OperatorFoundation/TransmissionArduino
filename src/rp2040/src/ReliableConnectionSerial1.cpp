#include "ReliableConnectionSerial1.h"

#include <Arduino.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <hardware/gpio.h>

ReliableConnectionSerial1* ReliableConnectionSerial1::instance = nullptr;

ReliableConnectionSerial1* ReliableConnectionSerial1::getInstance()
{
  if(!instance)
  {
    instance = new ReliableConnectionSerial1();
  }

  return instance;
}

void ReliableConnectionSerial1::uart0_handler()
{
  if(!instance)
  {
    return;
  }

  while(uart_is_readable(uart0))
  {
    uint8_t data = uart_getc(uart0);

    if(!instance->ring.put(data))
    {
      instance->buffer_full = true;
    }

    if(!instance->paused && instance->ring.shouldSendXOFF())
    {
      uart_putc_raw(uart0, 0x13); // XOFF
      instance->paused = true;
    }
  }
}

ReliableConnectionSerial1::ReliableConnectionSerial1()
{
  instance = this;
}

void ReliableConnectionSerial1::begin()
{
  if(!instance)
  {
    return;
  }

  uart_init(uart0, 115200);
  //uart_init(uart0, 1500000);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  delay(10); // Wait for initialization to take

  irq_set_priority(UART0_IRQ, 0xFF);
  irq_set_exclusive_handler(UART0_IRQ, uart0_handler);
  irq_set_enabled(UART0_IRQ, true);
  uart_set_irq_enables(uart0, true, false);

  if(instance->xonXoffEnabled)
  {
    uart_putc_raw(uart0, XON);
  }
}

void ReliableConnectionSerial1::enableXonXoff()
{
  xonXoffEnabled = true;
}

void ReliableConnectionSerial1::disableXonXoff()
{
  xonXoffEnabled = false;
}

int ReliableConnectionSerial1::tryReadOne()
{
  char c;
  if(ring.get(c))
  {
    return c;
  }
  else
  {
    return -1;
  }
}

char ReliableConnectionSerial1::readOne()
{
  char c;
  if(ring.get(c))
  {
    return c;
  }
  else
  {
    return 0; // FIXME - no way to fail because we're supposed to block, but we can't block anymore
  }
}

std::vector<char> ReliableConnectionSerial1::read()
{
  std::vector<char> results = std::vector<char>();

  // Drain the ring buffer
  char c;
  int count = 0;
  while(ring.get(c) && count < 128)
  {
    results.push_back(c);
    count++;
  }

  if(paused && ring.shouldSendXON())
  {
    uart_putc_raw(uart0, 0x11); // XON
    paused = false;
  }

  return results;
}

std::vector<char> ReliableConnectionSerial1::read(int size)
{
  std::vector<char> results = std::vector<char>();

  // Drain the ring buffer
  char c;
  int count = 0;
  while(ring.get(c) && count < size)
  {
    results.push_back(c);
    count++;
  }

  if(count < size)
  {
    results.resize(count);
  }

  return results;
}

void ReliableConnectionSerial1::write(std::vector<char> bs)
{
  for(auto c : bs)
  {
    uart_putc_raw(uart0, c);
  }
}

bool ReliableConnectionSerial1::availableForReading()
{
  return ring.available() > 0;
}
