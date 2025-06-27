// InterruptSafeRingBuffer.h
// A lock-free ring buffer safe for single producer (ISR) / single consumer (main)

#ifndef INTERRUPT_SAFE_RING_BUFFER_H
#define INTERRUPT_SAFE_RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>

template<typename T, size_t SIZE>
class InterruptSafeRingBuffer {
    static_assert((SIZE & (SIZE - 1)) == 0, "Size must be power of 2");
    
private:
    // Volatile arrays and indices for interrupt safety
    volatile T buffer[SIZE];
    volatile size_t head = 0;  // Written by producer (ISR)
    volatile size_t tail = 0;  // Written by consumer (main)
    
public:
    // Constructor
    InterruptSafeRingBuffer() = default;
    
    // Producer interface (call from ISR)
    bool put(const T& item) {
        size_t next_head = (head + 1) & (SIZE - 1);
        
        if (next_head == tail) {
            return false;  // Buffer full
        }
        
        buffer[head] = item;
        head = next_head;
        return true;
    }
    
    // Consumer interface (call from main)
    bool get(T& item) {
        if (head == tail) {
            return false;  // Buffer empty
        }
        
        item = buffer[tail];
        tail = (tail + 1) & (SIZE - 1);
        return true;
    }
    
    // Check if data available (safe from either context)
    bool available() const {
        return head != tail;
    }
    
    // Get number of items in buffer (safe from either context)
    size_t count() const {
        size_t h = head;
        size_t t = tail;
        return (h >= t) ? (h - t) : (SIZE - t + h);
    }
    
    // Check if buffer is full (safe from either context)
    bool full() const {
        return ((head + 1) & (SIZE - 1)) == tail;
    }
    
    // Check if buffer is empty (safe from either context)
    bool empty() const {
        return head == tail;
    }
    
    // Get total capacity
    static constexpr size_t capacity() {
        return SIZE;
    }
    
    // Get free space
    size_t free() const {
        return SIZE - count() - 1;  // -1 because we can't fill completely
    }
    
    // Peek at next item without removing (consumer only)
    bool peek(T& item) const {
        if (head == tail) {
            return false;
        }
        item = buffer[tail];
        return true;
    }
    
    // Clear buffer (consumer only - not safe from ISR)
    void clear() {
        tail = head;
    }
    
    // Get fill percentage (useful for flow control)
    uint8_t percentFull() const {
        return (count() * 100) / (SIZE - 1);
    }
};

// Convenience typedefs for common sizes
using RingBuffer256 = InterruptSafeRingBuffer<uint8_t, 256>;
using RingBuffer512 = InterruptSafeRingBuffer<uint8_t, 512>;
using RingBuffer1024 = InterruptSafeRingBuffer<uint8_t, 1024>;
using RingBuffer2048 = InterruptSafeRingBuffer<uint8_t, 2048>;
using RingBuffer4096 = InterruptSafeRingBuffer<uint8_t, 4096>;

#endif // INTERRUPT_SAFE_RING_BUFFER_H

// ===== Example usage in main.cpp =====
/*
#include "InterruptSafeRingBuffer.h"
#include <hardware/uart.h>
#include <hardware/irq.h>

// Create a 2048-byte ring buffer
InterruptSafeRingBuffer<uint8_t, 2048> uart_rx_buffer;

// Or use the typedef
// RingBuffer2048 uart_rx_buffer;

// Interrupt handler
void uart1_irq_handler() {
    while (uart_is_readable(uart1)) {
        uint8_t data = uart_getc(uart1);
        
        if (!uart_rx_buffer.put(data)) {
            // Buffer full - handle overflow
            // Could set a flag, increment counter, etc.
        }
    }
}

// Main loop
void loop() {
    uint8_t ch;
    while (uart_rx_buffer.get(ch)) {
        // Process character
        processVT100(ch);
    }
}

// Check buffer status
void checkStatus() {
    Serial.print("Buffer: ");
    Serial.print(uart_rx_buffer.count());
    Serial.print("/");
    Serial.print(uart_rx_buffer.capacity());
    Serial.print(" (");
    Serial.print(uart_rx_buffer.percentFull());
    Serial.println("% full)");
}
*/

// ===== Extended version with flow control support =====

template<typename T, size_t SIZE>
class FlowControlRingBuffer : public InterruptSafeRingBuffer<T, SIZE> {
private:
    using Base = InterruptSafeRingBuffer<T, SIZE>;
    
    // Flow control thresholds (customizable)
    const uint8_t high_water_mark;
    const uint8_t low_water_mark;
    
public:
    FlowControlRingBuffer(uint8_t high_percent = 75, uint8_t low_percent = 25)
        : high_water_mark(high_percent), low_water_mark(low_percent) {}
    
    bool shouldSendXOFF() const {
        return Base::percentFull() >= high_water_mark;
    }
    
    bool shouldSendXON() const {
        return Base::percentFull() <= low_water_mark;
    }
    
    bool isPanic() const {
        return Base::percentFull() >= 90;
    }
};

// ===== Usage example with flow control =====
/*
FlowControlRingBuffer<uint8_t, 2048> uart_buffer;
bool flow_stopped = false;

void uart_handler() {
    while (uart_is_readable(uart1)) {
        uint8_t data = uart_getc(uart1);
        
        if (!uart_buffer.put(data)) {
            // Overflow!
        }
        
        // Check flow control
        if (!flow_stopped && uart_buffer.shouldSendXOFF()) {
            uart_putc_raw(uart1, 0x13);  // XOFF
            flow_stopped = true;
        }
    }
}

void main_loop() {
    uint8_t ch;
    while (uart_buffer.get(ch)) {
        processChar(ch);
    }
    
    // Check if we should resume
    if (flow_stopped && uart_buffer.shouldSendXON()) {
        uart_putc_raw(uart1, 0x11);  // XON
        flow_stopped = false;
    }
}
*/