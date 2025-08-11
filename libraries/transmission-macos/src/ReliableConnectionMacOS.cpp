#include "ReliableConnectionMacOS.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <iostream>
#include <cstring>
#include <errno.h>

ReliableConnectionMacOS::ReliableConnectionMacOS(const std::string& device_path)
    : device_path(device_path), serial_fd(-1), running(false), xonXoffEnabled(false), 
      paused(false), buffer_full(false), ring(75, 25)
{
}

ReliableConnectionMacOS::~ReliableConnectionMacOS()
{
    end();
}

void ReliableConnectionMacOS::begin()
{
    if (running.load()) {
        return; // Already started
    }

    // Open the serial device
    serial_fd = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial_fd < 0) {
        std::cerr << "Failed to open " << device_path << ": " << strerror(errno) << std::endl;
        return;
    }

    if (!configureSerialPort()) {
        close(serial_fd);
        serial_fd = -1;
        return;
    }

    // Start the read thread
    running.store(true);
    read_thread = std::thread(&ReliableConnectionMacOS::readThreadFunction, this);

    // Send initial XON if flow control is enabled
    if (xonXoffEnabled.load()) {
        sendFlowControlChar(XON);
    }

    std::cout << "Serial connection established on " << device_path << std::endl;
}

void ReliableConnectionMacOS::end()
{
    if (!running.load()) {
        return;
    }

    running.store(false);

    if (read_thread.joinable()) {
        read_thread.join();
    }

    if (serial_fd >= 0) {
        close(serial_fd);
        serial_fd = -1;
    }
}

bool ReliableConnectionMacOS::configureSerialPort()
{
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(serial_fd, &tty) != 0) {
        std::cerr << "Error getting terminal attributes: " << strerror(errno) << std::endl;
        return false;
    }

    // Set baud rate (115200)
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    // 8-bit characters, no parity, 1 stop bit
    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;   // Clear size bits
    tty.c_cflag |= CS8;      // 8 bits per byte
    tty.c_cflag &= ~CRTSCTS; // Disable hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable reading and ignore modem control lines

    // Raw input mode
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control initially
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // Raw output mode
    tty.c_oflag &= ~OPOST;

    // No canonical processing
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Non-blocking read with minimal timeout
    tty.c_cc[VMIN] = 0;   // Minimum characters to read
    tty.c_cc[VTIME] = 1;  // Timeout in deciseconds (0.1 seconds)

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting terminal attributes: " << strerror(errno) << std::endl;
        return false;
    }

    // Flush any existing data
    tcflush(serial_fd, TCIOFLUSH);

    return true;
}

void ReliableConnectionMacOS::readThreadFunction()
{
    char buffer[256];
    fd_set read_fds;
    struct timeval timeout;

    while (running.load()) {
        FD_ZERO(&read_fds);
        FD_SET(serial_fd, &read_fds);
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms timeout

        int result = select(serial_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (result > 0 && FD_ISSET(serial_fd, &read_fds)) {
            int bytes_read = ::read(serial_fd, buffer, sizeof(buffer));
            
            if (bytes_read > 0) {
                for (int i = 0; i < bytes_read; i++) {
                    if (!ring.put(buffer[i])) {
                        buffer_full.store(true);
                        break;
                    }
                }

                // Check if we need to send XOFF
                if (!paused.load() && xonXoffEnabled.load() && ring.shouldSendXOFF()) {
                    sendFlowControlChar(XOFF);
                    paused.store(true);
                }
            }
            else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
                break;
            }
        }
        else if (result < 0 && errno != EINTR) {
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            break;
        }
    }
}

void ReliableConnectionMacOS::enableXonXoff()
{
    xonXoffEnabled.store(true);
    
    // Update terminal settings to enable software flow control
    if (serial_fd >= 0) {
        struct termios tty;
        if (tcgetattr(serial_fd, &tty) == 0) {
            tty.c_iflag |= (IXON | IXOFF);  // Enable software flow control
            tcsetattr(serial_fd, TCSANOW, &tty);
        }
    }
}

void ReliableConnectionMacOS::disableXonXoff()
{
    xonXoffEnabled.store(false);
    
    // Update terminal settings to disable software flow control
    if (serial_fd >= 0) {
        struct termios tty;
        if (tcgetattr(serial_fd, &tty) == 0) {
            tty.c_iflag &= ~(IXON | IXOFF);  // Disable software flow control
            tcsetattr(serial_fd, TCSANOW, &tty);
        }
    }
}

void ReliableConnectionMacOS::sendFlowControlChar(char c)
{
    std::lock_guard<std::mutex> lock(write_mutex);
    if (serial_fd >= 0) {
        ssize_t written = ::write(serial_fd, &c, 1);
        if (written < 0) {
            std::cerr << "Failed to send flow control character: " << strerror(errno) << std::endl;
        }
    }
}

int ReliableConnectionMacOS::tryReadOne()
{
    char c;
    if (ring.get(c)) {
        return static_cast<unsigned char>(c);
    }
    return -1;
}

char ReliableConnectionMacOS::readOne()
{
    char c;
    if (ring.get(c)) {
        return c;
    }
    return 0; // Non-blocking, return 0 if no data
}

std::vector<char> ReliableConnectionMacOS::read()
{
    std::vector<char> results;
    results.reserve(maxReadSize);

    char c;
    int count = 0;
    while (ring.get(c) && count < maxReadSize) {
        results.push_back(c);
        count++;
    }

    // Check if we should resume flow control
    if (paused.load() && xonXoffEnabled.load() && ring.shouldSendXON()) {
        sendFlowControlChar(XON);
        paused.store(false);
    }

    return results;
}

std::vector<char> ReliableConnectionMacOS::read(int size)
{
    std::vector<char> results;
    results.reserve(size);

    char c;
    int count = 0;
    while (ring.get(c) && count < size) {
        results.push_back(c);
        count++;
    }

    // Check if we should resume flow control
    if (paused.load() && xonXoffEnabled.load() && ring.shouldSendXON()) {
        sendFlowControlChar(XON);
        paused.store(false);
    }

    return results;
}

void ReliableConnectionMacOS::write(std::vector<char> bs)
{
    if (bs.empty() || serial_fd < 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(write_mutex);
    
    size_t total_written = 0;
    while (total_written < bs.size()) {
        ssize_t written = ::write(serial_fd, bs.data() + total_written, bs.size() - total_written);
        
        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Would block, try again
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            } else {
                std::cerr << "Write error: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        total_written += written;
    }
}
