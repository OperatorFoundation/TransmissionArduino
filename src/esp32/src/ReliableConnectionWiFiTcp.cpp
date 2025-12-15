//
// Created by Dr. Brandon Wiley on 12/15/25.
//

#include "ReliableConnectionWiFiTcp.h"

#include <Arduino.h>

Logger* ReliableConnectionWiFiTcp::logger = nullptr;

ReliableConnectionWiFiTcp::ReliableConnectionWiFiTcp(const char* host, uint16_t port)
    : host(host), port(port), connected(false)
{
}

ReliableConnectionWiFiTcp::~ReliableConnectionWiFiTcp()
{
    disconnect();
}

bool ReliableConnectionWiFiTcp::connect(const char* ssid, const char* password, unsigned long timeout_ms)
{
    if (logger) { logger->infof("Connecting to WiFi: %s", ssid); }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - start > timeout_ms)
        {
            if (logger) { logger->error("WiFi connection timeout"); }
            return false;
        }
        delay(100);
    }

    if (logger)
    {
        logger->infof("WiFi connected. IP: %s", WiFi.localIP().toString().c_str());
    }

    return true;
}

bool ReliableConnectionWiFiTcp::connectToHost()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (logger) { logger->error("WiFi not connected"); }
        return false;
    }

    if (logger) { logger->infof("Connecting to %s:%d", host, port); }

    if (!client.connect(host, port))
    {
        if (logger) { logger->error("TCP connection failed"); }
        connected = false;
        return false;
    }

    connected = true;
    if (logger) { logger->info("TCP connected"); }
    return true;
}

void ReliableConnectionWiFiTcp::disconnect()
{
    if (connected)
    {
        client.stop();
        connected = false;
        if (logger) { logger->info("TCP disconnected"); }
    }
}

bool ReliableConnectionWiFiTcp::isConnected()
{
    return connected && client.connected();
}

void ReliableConnectionWiFiTcp::fillRingBuffer()
{
    // Fill ring buffer from WiFi client
    while (client.available() > 0 && !ring.full())
    {
        int byte = client.read();
        if (byte >= 0)
        {
            if (logger) { logger->debugf("+0x%02X (%zu)", byte, ring.count() + 1); }
            if (!ring.put(static_cast<char>(byte)))
            {
                if (logger) { logger->debug("Ring buffer full"); }
                break;
            }
        }
    }
}

int ReliableConnectionWiFiTcp::tryReadOne()
{
    if (!isConnected())
    {
        return -1;
    }

    // First check ring buffer
    char c;
    if (ring.get(c))
    {
        return static_cast<unsigned char>(c);
    }

    // Try to fill from network
    fillRingBuffer();

    // Try ring buffer again
    if (ring.get(c))
    {
        return static_cast<unsigned char>(c);
    }

    return -1;
}

char ReliableConnectionWiFiTcp::readOne()
{
    if (!isConnected())
    {
        if (logger) { logger->error("Not connected in readOne()"); }
        return 0;
    }

    if (logger) { logger->debugf("readOne()"); }

    char c;
    while (true)
    {
        // Try to fill ring buffer from network
        fillRingBuffer();

        // Check ring buffer
        if (ring.get(c))
        {
            if (logger) { logger->debugf("-0x%02X (%zu)", static_cast<unsigned char>(c), ring.count()); }
            return c;
        }

        // Check connection status
        if (!isConnected())
        {
            if (logger) { logger->error("Connection lost in readOne()"); }
            return 0;
        }

        // Small delay to avoid busy-waiting
        delay(1);
    }
}

std::vector<char> ReliableConnectionWiFiTcp::read(int size)
{
    std::vector<char> results;
    results.reserve(size);

    if (!isConnected())
    {
        if (logger) { logger->error("Not connected in read()"); }
        return results;
    }

    if (logger) { logger->debugf("read(%d)", size); }

    // Block until we have 'size' bytes
    while (results.size() < static_cast<size_t>(size))
    {
        char c = readOne();
        results.push_back(c);

        if (!isConnected())
        {
            if (logger) { logger->debug("Connection lost during read()"); }
            break;
        }
    }

    return results;
}

void ReliableConnectionWiFiTcp::write(std::vector<char> bs)
{
    if (!isConnected())
    {
        if (logger) { logger->error("Not connected in write()"); }
        return;
    }

    if (!bs.empty())
    {
        size_t written = client.write(reinterpret_cast<const uint8_t*>(bs.data()), bs.size());

        if (logger && written != bs.size())
        {
            logger->debugf("Partial write: %zu/%zu bytes", written, bs.size());
        }
    }
}

bool ReliableConnectionWiFiTcp::availableForReading()
{
    if (!isConnected())
    {
        return false;
    }

    return (ring.count() > 0) || (client.available() > 0);
}