#ifndef _RELIABLE_CONNECTION_MACOS_H_
#define _RELIABLE_CONNECTION_MACOS_H_

#include <Connection.h>
#include <ring_buffer.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

class ReliableConnectionMacOS : public Connection
{
    public:
        static const char XON  = 0x11;
        static const char XOFF = 0x13;

        static const int maxBufferSize = 4096;
        static const int maxReadSize = 32;

        ReliableConnectionMacOS(const std::string& device_path = "/dev/tty.usbserial-0001");
        ~ReliableConnectionMacOS();

        void begin();
        void end();
        void enableXonXoff();
        void disableXonXoff();

        // Connection interface
        int tryReadOne() override;
        char readOne() override;
        std::vector<char> read(int size) override;
        void write(std::vector<char> bs) override;

        // Convenience method (not in base interface)
        std::vector<char> read();

    private:
        std::string device_path;
        int serial_fd;
        std::thread read_thread;
        std::atomic<bool> running;
        std::atomic<bool> xonXoffEnabled;
        std::atomic<bool> paused;
        std::atomic<bool> buffer_full;
        std::mutex write_mutex;

        FlowControlRingBuffer<char, maxBufferSize> ring;

        void readThreadFunction();
        bool configureSerialPort();
        void sendFlowControlChar(char c);
};

#endif