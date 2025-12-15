//
// Created by Dr. Brandon Wiley on 12/15/25.
//

#ifndef TRANSMISSION_CONNECTIONWIFITCP_H
#define TRANSMISSION_CONNECTIONWIFITCP_H

#include <Connection.h>
#include <ring_buffer.h>
#include <onda.h>
#include <WiFi.h>

class ReliableConnectionWiFiTcp : public Connection
{
  public:
    static const int maxBufferSize = 8192;  // Larger for WiFi bandwidth
    static const int maxReadSize = 1024;

    static Logger* logger;

    ReliableConnectionWiFiTcp(const char* host, uint16_t port);
    ~ReliableConnectionWiFiTcp();

    bool connect(const char* ssid, const char* password, unsigned long timeout_ms = 10000);
    bool connectToHost();
    void disconnect();
    bool isConnected();

    // Connection interface
    int tryReadOne() override;
    char readOne() override;
    std::vector<char> read(int size) override;
    void write(std::vector<char> bs) override;
    bool availableForReading() override;

  private:
    const char* host;
    uint16_t port;
    WiFiClient client;
    InterruptSafeRingBuffer<char, maxBufferSize> ring;
    bool connected;

    void fillRingBuffer();
};

#endif //TRANSMISSION_CONNECTIONWIFITCP_H