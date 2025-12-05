//
// Created by Dr. Brandon Wiley on 11/6/2025.
//

#ifndef RELIABLECONNECTIONSerial1_H
#define RELIABLECONNECTIONSerial1_H

#include <Connection.h>

class ReliableConnectionSerial1 : public Connection
{
  public:
    ReliableConnectionSerial1();
    ~ReliableConnectionSerial1() {}

    int tryReadOne();
    char readOne();
    std::vector<char> read();
    std::vector<char> read(int size);
    void write(std::vector<char> bs);
    bool availableForReading();
};

#endif //RELIABLECONNECTIONSerial1_H
