//
// Created by Dr. Brandon Wiley on 6/5/25.
//

#ifndef RELIABLECONNECTIONUSBCDC_H
#define RELIABLECONNECTIONUSBCDC_H

#include <Connection.h>

class ReliableConnectionUsbCdc : public Connection
{
  public:
    ReliableConnectionUsbCdc();
    ~ReliableConnectionUsbCdc() {}

    int tryReadOne();
    char readOne();
    std::vector<char> read();
    bytes read(int size);
    void write(std::vector<char> bs);
};

#endif //RELIABLECONNECTIONUSBCDC_H
