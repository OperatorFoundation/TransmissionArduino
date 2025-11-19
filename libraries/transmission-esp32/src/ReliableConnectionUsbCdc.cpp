//
// Created by Dr. Brandon Wiley on 11/19/25.
//

#include "ReliableConnectionUsbCdc.h"

#include <Arduino.h>

// Static members
ReliableConnectionUsbCdc* ReliableConnectionUsbCdc::instance = nullptr;

ReliableConnectionUsbCdc* ReliableConnectionUsbCdc::getInstance() {
    if (!instance) {
        instance = new ReliableConnectionUsbCdc();
    }
    return instance;
}

ReliableConnectionUsbCdc::ReliableConnectionUsbCdc() : ring(75, 25) {
    instance = this;
}

void ReliableConnectionUsbCdc::begin() {
    if (!instance) {
        return;
    }

    // Initialize USB CDC Serial
    Serial.begin(115200);  // Baud rate is mostly ignored for USB CDC but set anyway

    // Wait for USB connection (optional, remove if you don't want to wait)
    // while (!Serial) {
    //     delay(10);
    // }

    delay(100);  // Give USB stack time to initialize

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
    int result = tryReadOne();
    if (result >= 0) {
        return static_cast<char>(result);
    }
    return 0;
}

std::vector<char> ReliableConnectionUsbCdc::read() {
    std::vector<char> results;
    results.reserve(maxReadSize);

    // First, pull any available data from Serial into ring buffer
    while (Serial.available() > 0 && ring.available() < ring.capacity()) {
        int byte = Serial.read();
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

    // Drain the ring buffer
    char c;
    int count = 0;
    while (ring.get(c) && count < maxReadSize) {
        results.push_back(c);
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

    // First, pull any available data from Serial into ring buffer
    while (Serial.available() > 0 && ring.available() < ring.capacity() && results.size() < size) {
        int byte = Serial.read();
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

    // Drain the ring buffer
    char c;
    int count = 0;
    while (ring.get(c) && count < size) {
        results.push_back(c);
        count++;
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
    // Check both Serial buffer and ring buffer
    if (Serial.available() > 0) {
        // Pull data into ring buffer
        while (Serial.available() > 0 && ring.available() < ring.capacity()) {
            int byte = Serial.read();
            if (byte >= 0) {
                if (!ring.put(static_cast<char>(byte))) {
                    buffer_full = true;
                    break;
                }
            }
        }
    }

    return ring.available() > 0;
}