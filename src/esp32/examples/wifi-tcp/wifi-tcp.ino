// vnc-client-example.ino
#include <Arduino.h>
#include <ReliableConnectionWiFiTcp.h>

#include "wifi-secrets.h"

const char* VNC_HOST = "192.168.1.100";  // Your Pi/Linux host
const uint16_t VNC_PORT = 5900;

ReliableConnectionWiFiTcp vnc(VNC_HOST, VNC_PORT);

// VNC protocol state machine for debugging
enum VNCState {
    STATE_VERSION,
    STATE_SECURITY,
    STATE_SECURITY_RESULT,
    STATE_SERVER_INIT,
    STATE_RUNNING
};

VNCState currentState = STATE_VERSION;
std::vector<char> buffer;
unsigned long lastByteTime = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("VNC Client Starting");

    // Connect to WiFi
    if (!vnc.connect(WIFI_SSID, WIFI_PASSWORD))
    {
        Serial.println("WiFi connection failed!");
        return;
    }

    // Connect to VNC server
    if (!vnc.connectToHost())
    {
        Serial.println("VNC connection failed!");
        return;
    }

    Serial.println("Connected to VNC server");
}

void loop()
{
    if (!vnc.isConnected())
    {
        Serial.println("Connection lost, reconnecting...");
        delay(5000);
        vnc.connectToHost();
        return;
    }

    // Read VNC protocol messages
    if (vnc.availableForReading())
    {
        int byte = vnc.tryReadOne();
        if (byte >= 0)
        {
            // Process VNC protocol byte
            processVNCByte(byte);
        }
    }

    // Send VNC client messages (keyboard/mouse events, etc)
    // vnc.write(message);
}

void printHexDump(const std::vector<char>& data, const char* label)
{
    Serial.printf("\n=== %s (%zu bytes) ===\n", label, data.size());

    // Print hex
    for (size_t i = 0; i < data.size(); i++)
    {
        if (i % 16 == 0)
        {
            Serial.printf("%04X: ", i);
        }
        Serial.printf("%02X ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0 || i == data.size() - 1)
        {
            // Pad if not full line
            for (size_t j = (i % 16) + 1; j < 16; j++)
            {
                Serial.print("   ");
            }
            Serial.print(" | ");

            // Print ASCII
            size_t start = i - (i % 16);
            for (size_t j = start; j <= i; j++)
            {
                char c = data[j];
                Serial.print((c >= 32 && c < 127) ? c : '.');
            }
            Serial.println();
        }
    }
    Serial.println("===");
}

void printAsciiString(const std::vector<char>& data)
{
    Serial.print("ASCII: \"");
    for (char c : data)
    {
        if (c >= 32 && c < 127)
        {
            Serial.print(c);
        }
        else if (c == '\r')
        {
            Serial.print("\\r");
        }
        else if (c == '\n')
        {
            Serial.print("\\n");
        }
        else
        {
            Serial.printf("\\x%02X", (unsigned char)c);
        }
    }
    Serial.println("\"");
}

void processVNCByte(int byte)
{
    lastByteTime = millis();
    buffer.push_back((char)byte);

    // Real-time byte display
    Serial.printf("[%02X] ", (unsigned char)byte);
    if (byte >= 32 && byte < 127)
    {
        Serial.printf("'%c' ", byte);
    }

    switch (currentState)
    {
        case STATE_VERSION:
            // VNC version is 12 bytes: "RFB 003.008\n"
            if (buffer.size() == 12)
            {
                Serial.println("\n--- Server Version ---");
                printHexDump(buffer, "Version String");
                printAsciiString(buffer);

                // Parse version
                if (buffer[0] == 'R' && buffer[1] == 'F' && buffer[2] == 'B')
                {
                    int major = (buffer[4] - '0') * 100 + (buffer[5] - '0') * 10 + (buffer[6] - '0');
                    int minor = (buffer[8] - '0') * 100 + (buffer[9] - '0') * 10 + (buffer[10] - '0');
                    Serial.printf("VNC Version: %d.%d\n", major, minor);

                    // Client must respond with version
                    Serial.println("\n>>> CLIENT SHOULD SEND: RFB 003.008\\n");
                    std::string response = "RFB 003.008\n";
                    std::vector<char> resp(response.begin(), response.end());
                    vnc.write(resp);

                    buffer.clear();
                    currentState = STATE_SECURITY;
                    Serial.println("--- Waiting for Security Types ---");
                }
                else
                {
                    Serial.println("ERROR: Invalid RFB header!");
                    printHexDump(buffer, "Bad Header");
                }
            }
            break;

        case STATE_SECURITY:
            // First byte is number of security types
            if (buffer.size() == 1)
            {
                uint8_t numTypes = (uint8_t)buffer[0];
                Serial.printf("\nNumber of security types: %d\n", numTypes);

                if (numTypes == 0)
                {
                    Serial.println("ERROR: Server rejected connection (0 security types)");
                    Serial.println("Reason string should follow...");
                    buffer.clear();
                    // Will receive reason length (4 bytes) then reason string
                }
            }
            // After first byte, read that many security type bytes
            else if (buffer.size() > 1)
            {
                uint8_t numTypes = (uint8_t)buffer[0];
                if (buffer.size() == numTypes + 1)
                {
                    Serial.println("\n--- Security Types ---");
                    printHexDump(buffer, "Security Types");

                    for (size_t i = 1; i <= numTypes; i++)
                    {
                        uint8_t type = (uint8_t)buffer[i];
                        Serial.printf("Type %zu: %d = ", i, type);
                        switch (type)
                        {
                            case 0: Serial.println("Invalid"); break;
                            case 1: Serial.println("None (no auth)"); break;
                            case 2: Serial.println("VNC Authentication"); break;
                            case 5: Serial.println("RA2"); break;
                            case 6: Serial.println("RA2ne"); break;
                            case 16: Serial.println("Tight"); break;
                            case 17: Serial.println("Ultra"); break;
                            case 18: Serial.println("TLS"); break;
                            case 19: Serial.println("VeNCrypt"); break;
                            case 20: Serial.println("GTK-VNC SASL"); break;
                            case 21: Serial.println("MD5 hash"); break;
                            case 22: Serial.println("xvp"); break;
                            default: Serial.println("Unknown"); break;
                        }
                    }

                    // For debugging, select "None" (type 1) if available
                    bool foundNone = false;
                    for (size_t i = 1; i <= numTypes; i++)
                    {
                        if (buffer[i] == 1)
                        {
                            foundNone = true;
                            break;
                        }
                    }

                    if (foundNone)
                    {
                        Serial.println("\n>>> CLIENT SHOULD SEND: Security Type 1 (None)");
                        std::vector<char> secType = {1};
                        vnc.write(secType);

                        buffer.clear();
                        currentState = STATE_SECURITY_RESULT;
                        Serial.println("--- Waiting for Security Result ---");
                    }
                    else
                    {
                        Serial.println("\nWARNING: 'None' security not available. Stopping.");
                        Serial.println("You'll need to implement authentication.");
                    }
                }
            }
            break;

        case STATE_SECURITY_RESULT:
            // Security result is 4 bytes (uint32_t)
            if (buffer.size() == 4)
            {
                Serial.println("\n--- Security Result ---");
                printHexDump(buffer, "Security Result");

                uint32_t result = ((uint8_t)buffer[0] << 24) |
                                ((uint8_t)buffer[1] << 16) |
                                ((uint8_t)buffer[2] << 8) |
                                ((uint8_t)buffer[3]);

                Serial.printf("Result: %u = ", result);
                if (result == 0)
                {
                    Serial.println("OK (Success!)");

                    // Send ClientInit (shared flag)
                    Serial.println("\n>>> CLIENT SHOULD SEND: ClientInit (shared=1)");
                    std::vector<char> clientInit = {1}; // 1 = shared
                    vnc.write(clientInit);

                    buffer.clear();
                    currentState = STATE_SERVER_INIT;
                    Serial.println("--- Waiting for ServerInit ---");
                }
                else
                {
                    Serial.println("FAILED");
                    Serial.println("Reason string should follow...");
                }
            }
            break;

        case STATE_SERVER_INIT:
            // ServerInit is complex: width(2) + height(2) + PixelFormat(16) + name-length(4) + name
            // Minimum 24 bytes before name
            if (buffer.size() >= 24)
            {
                uint32_t nameLength = ((uint8_t)buffer[20] << 24) |
                                     ((uint8_t)buffer[21] << 16) |
                                     ((uint8_t)buffer[22] << 8) |
                                     ((uint8_t)buffer[23]);

                if (buffer.size() >= 24 + nameLength)
                {
                    Serial.println("\n--- Server Init ---");
                    printHexDump(buffer, "ServerInit");

                    uint16_t width = ((uint8_t)buffer[0] << 8) | (uint8_t)buffer[1];
                    uint16_t height = ((uint8_t)buffer[2] << 8) | (uint8_t)buffer[3];

                    Serial.printf("\nFramebuffer: %dx%d\n", width, height);

                    // Pixel format details
                    uint8_t bitsPerPixel = buffer[4];
                    uint8_t depth = buffer[5];
                    uint8_t bigEndian = buffer[6];
                    uint8_t trueColor = buffer[7];
                    uint16_t redMax = ((uint8_t)buffer[8] << 8) | (uint8_t)buffer[9];
                    uint16_t greenMax = ((uint8_t)buffer[10] << 8) | (uint8_t)buffer[11];
                    uint16_t blueMax = ((uint8_t)buffer[12] << 8) | (uint8_t)buffer[13];
                    uint8_t redShift = buffer[14];
                    uint8_t greenShift = buffer[15];
                    uint8_t blueShift = buffer[16];

                    Serial.printf("Pixel Format:\n");
                    Serial.printf("  Bits per pixel: %d\n", bitsPerPixel);
                    Serial.printf("  Depth: %d\n", depth);
                    Serial.printf("  Big endian: %d\n", bigEndian);
                    Serial.printf("  True color: %d\n", trueColor);
                    Serial.printf("  RGB Max: R=%d G=%d B=%d\n", redMax, greenMax, blueMax);
                    Serial.printf("  RGB Shift: R=%d G=%d B=%d\n", redShift, greenShift, blueShift);

                    // Desktop name
                    std::vector<char> name(buffer.begin() + 24, buffer.begin() + 24 + nameLength);
                    Serial.print("Desktop name: \"");
                    for (char c : name)
                    {
                        Serial.print((c >= 32 && c < 127) ? c : '?');
                    }
                    Serial.println("\"");

                    buffer.clear();
                    currentState = STATE_RUNNING;
                    Serial.println("\n=== Handshake Complete! ===");
                    Serial.println("Now in running state. You can send FramebufferUpdateRequest.");
                }
            }
            break;

        case STATE_RUNNING:
            // Just dump bytes in running state
            if (buffer.size() >= 16 || (millis() - lastByteTime > 100 && !buffer.empty()))
            {
                printHexDump(buffer, "Server Message");
                buffer.clear();
            }
            break;
    }
}