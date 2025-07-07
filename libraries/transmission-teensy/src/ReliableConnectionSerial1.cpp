#include "ReliableConnectionSerial1.h"
#include <Arduino.h>

ReliableConnectionSerial1* ReliableConnectionSerial1::instance = nullptr;

ReliableConnectionSerial1::ReliableConnectionSerial1() {
    instance = this;
}

ReliableConnectionSerial1* ReliableConnectionSerial1::getInstance() {
    if (!instance) {
        instance = new ReliableConnectionSerial1();
    }
    return instance;
}

void ReliableConnectionSerial1::begin() {
    if (!instance) {
        return;
    }

    // Initialize Serial1 at 115200 baud
    Serial1.begin(115200);
    // For higher speed: Serial1.begin(1500000);

    delay(10); // Wait for initialization to take

    // Clear any pending data
    while (Serial1.available()) {
        Serial1.read();
    }

    // Initialize state
    paused = false;
    buffer_full = false;

    // Send initial XON if XON/XOFF is enabled
    if (instance->xonXoffEnabled) {
        Serial1.write(XON);
    }
}

// This function is automatically called by Teensy when Serial1 receives data
void ReliableConnectionSerial1::serialEvent1() {
    if (!instance) {
        return;
    }

    while (Serial1.available()) {
        uint8_t data = Serial1.read();

        if (!instance->ring.put(data)) {
            instance->buffer_full = true;
        }

        if (!instance->paused && instance->ring.shouldSendXOFF()) {
            Serial1.write(0x13); // XOFF
            instance->paused = true;
        }
    }
}

void ReliableConnectionSerial1::enableXonXoff() {
    xonXoffEnabled = true;
}

void ReliableConnectionSerial1::disableXonXoff() {
    xonXoffEnabled = false;
}

int ReliableConnectionSerial1::tryReadOne() {
    char c;
    if (ring.get(c)) {
        return c;
    } else {
        return -1;
    }
}

char ReliableConnectionSerial1::readOne() {
    char c;
    if (ring.get(c)) {
        return c;
    } else {
        return 0; // FIXME - no way to fail because we're supposed to block, but we can't block anymore
    }
}

std::vector<char> ReliableConnectionSerial1::read() {
    std::vector<char> results = std::vector<char>();

    // Drain the ring buffer
    char c;
    int count = 0;
    while (ring.get(c) && count < 128) {
        results.push_back(c);
        count++;
    }

    if (paused && ring.shouldSendXON()) {
        Serial1.write(0x11); // XON
        paused = false;
    }

    return results;
}

std::vector<char> ReliableConnectionSerial1::read(int size) {
    std::vector<char> results = std::vector<char>();

    // Drain the ring buffer
    char c;
    int count = 0;
    while (ring.get(c) && count < size) {
        results.push_back(c);
        count++;
    }

    if (count < size) {
        results.resize(count);
    }

    return results;
}

void ReliableConnectionSerial1::write(std::vector<char> bs) {
    for (auto c : bs) {
        Serial1.write(c);
    }
}