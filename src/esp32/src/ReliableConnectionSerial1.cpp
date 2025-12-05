#include "ReliableConnectionSerial1.h"
#include <Arduino.h>
#include <driver/uart.h>

// ESP32-S3 specific configuration
static const uart_port_t UART_PORT = UART_NUM_1;
static const int UART_TX_PIN = 17;  // Adjust for your hardware
static const int UART_RX_PIN = 18;  // Adjust for your hardware
static const int ESP_BUF_SIZE = 2048;

QueueHandle_t ReliableConnectionSerial1::uart_queue = nullptr;

ReliableConnectionSerial1::ReliableConnectionSerial1() : ring(75, 25) {}

// ESP32-S3 uses task-based UART handling instead of direct ISR
void ReliableConnectionSerial1::uart0_handler() {
    // On ESP32, this is handled by the UART event task
    // This function exists for API compatibility

    if (!uart_queue) {
        return;
    }

    // Process any pending UART data in the hardware buffer
    uint8_t data;
    size_t buffered_size = 0;
    uart_get_buffered_data_len(UART_PORT, &buffered_size);

    while (buffered_size > 0) {
        if (uart_read_bytes(UART_PORT, &data, 1, 0) > 0) {
            if (!instance->ring.put(static_cast<char>(data))) {
                instance->buffer_full = true;
            }

            // Check flow control
            if (!instance->paused && instance->xonXoffEnabled &&
                instance->ring.shouldSendXOFF()) {
                char xoff = XOFF;
                uart_write_bytes(UART_PORT, &xoff, 1);
                instance->paused = true;
            }
        }
        uart_get_buffered_data_len(UART_PORT, &buffered_size);
    }
}

// UART event handling task
static void uart_event_task(void* pvParameters) {
    uart_event_t event;

    while (true) {
        if (xQueueReceive(ReliableConnectionSerial1::uart_queue, (void*)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    // Call the handler which has access to private members
                    ReliableConnectionSerial1::uart0_handler();
                    break;

                case UART_FIFO_OVF:
                    uart_flush_input(UART_PORT);
                    xQueueReset(ReliableConnectionSerial1::uart_queue);
                    break;

                case UART_BUFFER_FULL:
                    uart_flush_input(UART_PORT);
                    xQueueReset(ReliableConnectionSerial1::uart_queue);
                    break;

                default:
                    break;
            }
        }
    }

    vTaskDelete(NULL);
}

void ReliableConnectionSerial1::begin() {
    if (!instance) {
        return;
    }

    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;  // Change to 1500000 for high speed
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    // Install UART driver with event queue
    uart_driver_install(UART_PORT, ESP_BUF_SIZE * 2, ESP_BUF_SIZE * 2, 20, &uart_queue, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Set UART interrupt threshold for fast response
    uart_set_rx_full_threshold(UART_PORT, 1);

    // Create high-priority UART event handling task
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 12, NULL);

    // Wait for initialization
    delay(10);

    if (xonXoffEnabled) {
        char xon = XON;
        uart_write_bytes(UART_PORT, &xon, 1);
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
        return static_cast<unsigned char>(c);
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
    std::vector<char> results;
    results.reserve(maxReadSize);

    // Drain the ring buffer
    char c;
    int count = 0;
    while (ring.get(c) && count < maxReadSize) {
        results.push_back(c);
        count++;
    }

    // Check if we should resume flow
    if (paused && xonXoffEnabled && ring.shouldSendXON()) {
        char xon = XON;
        uart_write_bytes(UART_PORT, &xon, 1);
        paused = false;
    }

    return results;
}

std::vector<char> ReliableConnectionSerial1::read(int size) {
    std::vector<char> results;
    results.reserve(size);

    // Drain the ring buffer
    char c;
    int count = 0;
    while (ring.get(c) && count < size) {
        results.push_back(static_cast<uint8_t>(c));
        count++;
    }

    // Check if we should resume flow
    if (paused && xonXoffEnabled && ring.shouldSendXON()) {
        char xon = XON;
        uart_write_bytes(UART_PORT, &xon, 1);
        paused = false;
    }

    return results;
}

void ReliableConnectionSerial1::write(std::vector<char> bs) {
    uart_write_bytes(UART_PORT, bs.data(), bs.size());
}

bool ReliableConnectionSerial1::availableForReading()
{
    return ring.available() > 0;
}
