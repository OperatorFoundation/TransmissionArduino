//
// Created by Dr. Brandon Wiley on 12/15/25.
//

#ifndef TRANSMISSION_RELIABLECONNECTIONTCPMACOS_H
#define TRANSMISSION_RELIABLECONNECTIONTCPMACOS_H

#include <string>
#include <thread>
#include <atomic>
#include <mutex>

#include <Connection.h>
#include <ring_buffer.h>

class ReliableConnectionTcpMacOS : public Connection
{
  public:
    static const int maxBufferSize = 8192;
    static const int maxReadSize = 1024;

    ReliableConnectionTcpMacOS(const std::string& host, uint16_t port);
    ~ReliableConnectionTcpMacOS();

    bool connect();
    void disconnect();
    bool isConnected();
    void setDebugMode(bool enable);

    // Connection interface
    int tryReadOne() override;
    char readOne() override;
    std::vector<char> read(int size) override;
    void write(std::vector<char> bs) override;
    bool availableForReading() override;

    // Convenience method (not in base interface)
    std::vector<char> read();

  private:
    bool debug_mode = false;
    std::string host;
    uint16_t port;
    int socket_fd;
    std::thread read_thread;
    std::atomic<bool> running;
    std::atomic<bool> connected;
    std::mutex write_mutex;

    InterruptSafeRingBuffer<char, maxBufferSize> ring;

    void readThreadFunction();
};

#endif //TRANSMISSION_RELIABLECONNECTIONTCPMACOS_H