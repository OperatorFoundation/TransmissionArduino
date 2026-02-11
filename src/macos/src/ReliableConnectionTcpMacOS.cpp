//
// Created by Dr. Brandon Wiley on 12/15/25.
//

#include "ReliableConnectionTcpMacOS.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <iostream>
#include <cstring>
#include <errno.h>

ReliableConnectionTcpMacOS::ReliableConnectionTcpMacOS(const std::string& host, uint16_t port)
    : host(host), port(port), socket_fd(-1), running(false), connected(false)
{
}

ReliableConnectionTcpMacOS::~ReliableConnectionTcpMacOS()
{
    disconnect();
}

bool ReliableConnectionTcpMacOS::connect()
{
    if (connected.load()) {
        return true; // Already connected
    }

    // Create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Resolve hostname
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        std::cerr << "Failed to resolve host: " << host << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // Connect to server
    if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to " << host << ":" << port
                  << " - " << strerror(errno) << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);

    // Start read thread
    connected.store(true);
    running.store(true);
    read_thread = std::thread(&ReliableConnectionTcpMacOS::readThreadFunction, this);

    std::cout << "TCP connection established to " << host << ":" << port << std::endl;
    return true;
}

void ReliableConnectionTcpMacOS::disconnect()
{
    if (!running.load()) {
        return;
    }

    running.store(false);
    connected.store(false);

    if (read_thread.joinable()) {
        read_thread.join();
    }

    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }

    std::cout << "TCP connection closed" << std::endl;
}

bool ReliableConnectionTcpMacOS::isConnected()
{
    return connected.load();
}

void ReliableConnectionTcpMacOS::readThreadFunction()
{
    char buffer[2048];  // Larger buffer for network data
    fd_set read_fds;
    struct timeval timeout;

    while (running.load()) {
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms timeout

        int result = select(socket_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (result > 0 && FD_ISSET(socket_fd, &read_fds)) {
            ssize_t bytes_read = ::read(socket_fd, buffer, sizeof(buffer));

            if (bytes_read > 0) {
                // Debug logging
                if (debug_mode) {
                    std::cout << "TCP recv (" << bytes_read << " bytes): ";
                    for (ssize_t i = 0; i < bytes_read && i < 50; i++) {
                        unsigned char c = buffer[i];
                        if (c >= 32 && c < 127) {
                            std::cout << (char)c;
                        } else {
                            std::cout << "<0x" << std::hex << (int)c << std::dec << ">";
                        }
                    }
                    if (bytes_read > 50) std::cout << "...";
                    std::cout << std::endl;
                }

                // Put data in ring buffer
                for (ssize_t i = 0; i < bytes_read; i++) {
                    if (!ring.put(buffer[i])) {
                        std::cerr << "Ring buffer full! Dropping data." << std::endl;
                        break;
                    }
                }
            }
            else if (bytes_read == 0) {
                // Connection closed by peer
                std::cerr << "Connection closed by remote host" << std::endl;
                connected.store(false);
                break;
            }
            else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
                connected.store(false);
                break;
            }
        }
        else if (result < 0 && errno != EINTR) {
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            connected.store(false);
            break;
        }
    }
}

int ReliableConnectionTcpMacOS::tryReadOne()
{
    char c;
    if (ring.get(c)) {
        return static_cast<unsigned char>(c);
    }
    return -1;
}

char ReliableConnectionTcpMacOS::readOne()
{
    char c;

    // Block until we get a character or connection dies
    while (connected.load()) {
        if (ring.get(c)) {
            return c;
        }
        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0; // Connection lost
}

std::vector<char> ReliableConnectionTcpMacOS::read()
{
    std::vector<char> results;
    results.reserve(maxReadSize);

    char c;
    int count = 0;

    while (ring.get(c) && count < maxReadSize) {
        results.push_back(c);
        count++;
    }

    return results;
}

std::vector<char> ReliableConnectionTcpMacOS::read(int size)
{
    std::vector<char> results;
    results.reserve(size);

    char c;
    while (results.size() < static_cast<size_t>(size) && connected.load()) {
        if (ring.get(c)) {
            results.push_back(c);
        } else {
            // Wait a bit for more data
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return results;
}

void ReliableConnectionTcpMacOS::write(std::vector<char> bs)
{
    if (bs.empty() || socket_fd < 0 || !connected.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(write_mutex);

    size_t total_written = 0;
    while (total_written < bs.size()) {
        ssize_t written = ::write(socket_fd, bs.data() + total_written,
                                   bs.size() - total_written);

        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Would block, try again
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            } else {
                std::cerr << "Write error: " << strerror(errno) << std::endl;
                connected.store(false);
                break;
            }
        }

        total_written += written;
    }

    if (debug_mode && total_written > 0) {
        std::cout << "TCP sent (" << total_written << " bytes)" << std::endl;
    }
}

void ReliableConnectionTcpMacOS::setDebugMode(bool enable)
{
    debug_mode = enable;
}

bool ReliableConnectionTcpMacOS::availableForReading()
{
    return ring.count() > 0;
}