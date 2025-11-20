//
// Created by Dr. Brandon Wiley on 11/19/25.
//

#ifndef EDEN_RELIABLECONNECTIONUSBCDC_H
#define EDEN_RELIABLECONNECTIONUSBCDC_H

#include <Connection.h>

#include <ring_buffer.h>

class ReliableConnectionUsbCdc : public Connection
{
  public:
    static const char XON  = 0x11;
    static const char XOFF = 0x13;

    static const int maxBufferSize = 4096;
    static const int maxReadSize = 32;

    static void uart0_handler();

    ReliableConnectionUsbCdc();
    ~ReliableConnectionUsbCdc() {}

    void begin();
    void enableXonXoff();
    void disableXonXoff();

    // Connection
    int tryReadOne();
    char readOne();
    std::vector<char> read();
    std::vector<char> read(int size);
    void write(std::vector<char> bs);
    bool availableForReading();
    // end Connection

  private:
    bool xonXoffEnabled = false;
    FlowControlRingBuffer<char, maxBufferSize> ring;
    volatile bool paused = false;
    volatile bool buffer_full = false;
};

#endif //EDEN_RELIABLECONNECTIONUSBCDC_H