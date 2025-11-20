//
// Created by Dr. Brandon Wiley on 11/19/25.
//

#include "ReliableConnectionUsbCdc.h"

#include <Arduino.h>

Logger* ReliableConnectionUsbCdc::logger = nullptr;

ReliableConnectionUsbCdc::ReliableConnectionUsbCdc() : ring(75, 25) {}

void ReliableConnectionUsbCdc::begin() {
    if (xonXoffEnabled) {
        char xon = XON;
        Serial.write(xon);
    }
}

void ReliableConnectionUsbCdc::enableXonXoff() {
    xonXoffEnabled = true;
}

void ReliableConnectionUsbCdc::disableXonXoff() {
    xonXoffEnabled = false;
}

int ReliableConnectionUsbCdc::tryReadOne() {
    // First check ring buffer
    char c;
    if (ring.get(c)) {
        return static_cast<unsigned char>(c);
    }

    // Try to fill ring buffer from Serial
    while (Serial.available() > 0) {
        int byte = Serial.read();
		if(logger) { logger->debugf("+0x%08X (%d)", byte, ring.count()+1); }
        if (byte >= 0) {
            if (!ring.put(static_cast<char>(byte))) {
                buffer_full = true;
                break;
            }

            // Check flow control
            if (!paused && xonXoffEnabled && ring.shouldSendXOFF()) {
                Serial.write(XOFF);
                paused = true;
            }
        }
    }

    // Try again from ring buffer
    if (ring.get(c)) {
        return static_cast<unsigned char>(c);
    }

    return -1;
}

char ReliableConnectionUsbCdc::readOne() {
    char c;

    if(logger) { logger->debugf("readOne()"); }

    // Block until we get a character
    while (true) {
        // Try to fill ring buffer from Serial
        while (Serial.available() > 0 && ring.available() < ring.capacity()) {
            int byte = Serial.read();
            if (byte >= 0) {
                if(logger) { logger->debugf("+0x%08X (%d)", byte, ring.count()+1); }
                if (!ring.put(static_cast<char>(byte))) {
                    buffer_full = true;
                    break;
                }

                // Check flow control
                if (!paused && xonXoffEnabled && ring.shouldSendXOFF()) {
                    Serial.write(XOFF);
                    paused = true;
                }
            }
            else
            {
                if(logger) { logger->debugf("X"); }
            }
        }

        // Check ring buffer after filling
        if (ring.get(c)) {
            if(logger) { logger->debugf("-0x%08X (%d)", c, ring.count()); }
            return c;
        }

        // Small delay to avoid busy-waiting and burning CPU
        delay(1);  // or delayMicroseconds(100) for faster response
    }
}

std::vector<char> ReliableConnectionUsbCdc::read() {
    std::vector<char> results;
    results.reserve(maxReadSize);

    if(logger) { logger->debugf("read()"); }

    // First, pull any available data from Serial into ring buffer
    while (Serial.available() > 0 && ring.available() < ring.capacity()) {
        int byte = Serial.read();
		if(logger) { logger->debugf("+0x%08X (%d)", byte, ring.count()+1); }
        if (byte >= 0) {
            if (!ring.put(static_cast<char>(byte))) {
                buffer_full = true;
                if(logger) { logger->debugf("!"); }
                break;
            }

            // Check flow control
            if (!paused && xonXoffEnabled && ring.shouldSendXOFF()) {
                Serial.write(XOFF);
                paused = true;
            }
        }
    }

    // Drain the ring buffer
    char c;
    int count = 0;
    while (ring.get(c) && count < maxReadSize) {
        results.push_back(c);
        logger->debugf("-0x%08X (%d)", c, ring.count());
        count++;
    }

    // Check if we should resume flow
    if (paused && xonXoffEnabled && ring.shouldSendXON()) {
        Serial.write(XON);
        paused = false;
    }

    return results;
}

std::vector<char> ReliableConnectionUsbCdc::read(int size) {
    std::vector<char> results;
    results.reserve(size);

    if(logger) { logger->debugf("read(%d)\n", size); }

    // Block until we have 'size' bytes
    while (results.size() < size) {
        // Pull any available data from Serial into ring buffer
        while (Serial.available() > 0 && ring.available() < ring.capacity()) {
            int byte = Serial.read();
            if(logger) { logger->debugf("+0x%08X (%d)", byte, ring.count()+1); }
            if (byte >= 0) {
                if (!ring.put(static_cast<char>(byte))) {
                    buffer_full = true;
                    if(logger) { logger->debugf("!"); }
                    break;
                }

                // Check flow control
                if (!paused && xonXoffEnabled && ring.shouldSendXOFF()) {
                    Serial.write(XOFF);
                    paused = true;
                }
            }
            else
            {
                if(logger) { logger->debugf("X"); }
            }
        }

        // Drain the ring buffer
        char c;
        while (ring.get(c) && results.size() < size) {
            results.push_back(c);
            if(logger) { logger->debugf("-0x%08X (%d)", c, ring.count()); }
        }

        // If we still don't have enough, wait a bit
        if (results.size() < size) {
            yield();  // Let other tasks run
        }
    }

    // Check if we should resume flow
    if (paused && xonXoffEnabled && ring.shouldSendXON()) {
        Serial.write(XON);
        paused = false;
    }

    return results;
}

void ReliableConnectionUsbCdc::write(std::vector<char> bs) {
    if (!bs.empty()) {
        Serial.write(reinterpret_cast<const uint8_t*>(bs.data()), bs.size());
    }
}

bool ReliableConnectionUsbCdc::availableForReading()
{
    // Just check if there's data available - don't consume it!
    return (ring.available() > 0) || (Serial.available() > 0);
}