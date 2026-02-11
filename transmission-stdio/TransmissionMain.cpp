// TransmissionMain.cpp
#include "TransmissionMain.h"
#include <iostream>
#include <iomanip>
#include <chrono>

TransmissionMain::TransmissionMain(const std::string& host, uint16_t port)
    : vnc(host, port), currentState(STATE_VERSION), lastByteTime(0)
{
}

TransmissionMain::~TransmissionMain()
{
    vnc.disconnect();
}

bool TransmissionMain::connect()
{
    std::cout << "\n=== VNC Protocol Debugger (macOS) ===" << std::endl;
    std::cout << "Connecting to VNC server..." << std::endl;
    
    vnc.setDebugMode(false);  // Set to true for TCP-level debugging
    
    if (!vnc.connect()) {
        std::cerr << "Failed to connect!" << std::endl;
        return false;
    }
    
    std::cout << "Connected! Waiting for server version...\n" << std::endl;
    return true;
}

void TransmissionMain::loop()
{
    if (!vnc.isConnected()) {
        std::cerr << "\n!!! Connection lost !!!" << std::endl;
        return;
    }

    if (vnc.availableForReading()) {
        int byte = vnc.tryReadOne();
        if (byte >= 0) {
            processVNCByte(byte);
        }
    }
}

void TransmissionMain::printHexDump(const std::vector<char>& data, const char* label)
{
    std::cout << "\n=== " << label << " (" << data.size() << " bytes) ===" << std::endl;
    
    // Print hex
    for (size_t i = 0; i < data.size(); i++) {
        if (i % 16 == 0) {
            std::cout << std::setfill('0') << std::setw(4) << std::hex << i << ": ";
        }
        std::cout << std::setfill('0') << std::setw(2) << std::hex 
                  << (int)(unsigned char)data[i] << " ";
        
        if ((i + 1) % 16 == 0 || i == data.size() - 1) {
            // Pad if not full line
            for (size_t j = (i % 16) + 1; j < 16; j++) {
                std::cout << "   ";
            }
            std::cout << " | ";
            
            // Print ASCII
            size_t start = i - (i % 16);
            for (size_t j = start; j <= i; j++) {
                char c = data[j];
                std::cout << (c >= 32 && c < 127 ? c : '.');
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::dec << "===" << std::endl;
}

void TransmissionMain::printAsciiString(const std::vector<char>& data)
{
    std::cout << "ASCII: \"";
    for (char c : data) {
        if (c >= 32 && c < 127) {
            std::cout << c;
        } else if (c == '\r') {
            std::cout << "\\r";
        } else if (c == '\n') {
            std::cout << "\\n";
        } else {
            std::cout << "\\x" << std::hex << std::setfill('0') << std::setw(2) 
                      << (int)(unsigned char)c << std::dec;
        }
    }
    std::cout << "\"" << std::endl;
}

void TransmissionMain::handleVersionState()
{
    if (buffer.size() == 12) {
        std::cout << "\n--- Server Version ---" << std::endl;
        printHexDump(buffer, "Version String");
        printAsciiString(buffer);
        
        // Parse version
        if (buffer[0] == 'R' && buffer[1] == 'F' && buffer[2] == 'B') {
            int major = (buffer[4] - '0') * 100 + (buffer[5] - '0') * 10 + (buffer[6] - '0');
            int minor = (buffer[8] - '0') * 100 + (buffer[9] - '0') * 10 + (buffer[10] - '0');
            std::cout << "VNC Version: " << major << "." << minor << std::endl;
            
            // Client responds with version
            std::cout << "\n>>> CLIENT SENDING: RFB 003.008\\n" << std::endl;
            std::string response = "RFB 003.008\n";
            std::vector<char> resp(response.begin(), response.end());
            vnc.write(resp);
            
            buffer.clear();
            currentState = STATE_SECURITY;
            std::cout << "--- Waiting for Security Types ---" << std::endl;
        } else {
            std::cout << "ERROR: Invalid RFB header!" << std::endl;
            printHexDump(buffer, "Bad Header");
        }
    }
}

void TransmissionMain::handleSecurityState()
{
    if (buffer.size() == 1) {
        uint8_t numTypes = (uint8_t)buffer[0];
        std::cout << "\nNumber of security types: " << (int)numTypes << std::endl;
        
        if (numTypes == 0) {
            std::cout << "ERROR: Server rejected connection (0 security types)" << std::endl;
            std::cout << "Reason string should follow..." << std::endl;
            buffer.clear();
        }
    } else if (buffer.size() > 1) {
        uint8_t numTypes = (uint8_t)buffer[0];
        if (buffer.size() == numTypes + 1) {
            std::cout << "\n--- Security Types ---" << std::endl;
            printHexDump(buffer, "Security Types");
            
            for (size_t i = 1; i <= numTypes; i++) {
                uint8_t type = (uint8_t)buffer[i];
                std::cout << "Type " << i << ": " << (int)type << " = ";
                switch (type) {
                    case 0: std::cout << "Invalid"; break;
                    case 1: std::cout << "None (no auth)"; break;
                    case 2: std::cout << "VNC Authentication"; break;
                    case 5: std::cout << "RA2"; break;
                    case 16: std::cout << "Tight"; break;
                    case 18: std::cout << "TLS"; break;
                    case 19: std::cout << "VeNCrypt"; break;
                    default: std::cout << "Unknown"; break;
                }
                std::cout << std::endl;
            }

            // Select "None" (type 1) if available
            bool foundNone = false;
            for (size_t i = 1; i <= numTypes; i++) {
                if (buffer[i] == 1) {
                    foundNone = true;
                    break;
                }
            }

            if (foundNone) {
                std::cout << "\n>>> CLIENT SENDING: Security Type 1 (None)" << std::endl;
                std::vector<char> secType = {1};
                vnc.write(secType);

                buffer.clear();
                currentState = STATE_SECURITY_RESULT;
                std::cout << "--- Waiting for Security Result ---" << std::endl;
            } else {
                std::cout << "\nWARNING: 'None' security not available. Need authentication." << std::endl;
            }
        }
    }
}

void TransmissionMain::handleSecurityResultState()
{
    if (buffer.size() == 4) {
        std::cout << "\n--- Security Result ---" << std::endl;
        printHexDump(buffer, "Security Result");

        uint32_t result = ((uint8_t)buffer[0] << 24) |
                        ((uint8_t)buffer[1] << 16) |
                        ((uint8_t)buffer[2] << 8) |
                        ((uint8_t)buffer[3]);

        std::cout << "Result: " << result << " = ";
        if (result == 0) {
            std::cout << "OK (Success!)" << std::endl;

            // Send ClientInit (shared flag)
            std::cout << "\n>>> CLIENT SENDING: ClientInit (shared=1)" << std::endl;
            std::vector<char> clientInit = {1};
            vnc.write(clientInit);

            buffer.clear();
            currentState = STATE_SERVER_INIT;
            std::cout << "--- Waiting for ServerInit ---" << std::endl;
        } else {
            std::cout << "FAILED" << std::endl;
            std::cout << "Reason string should follow..." << std::endl;
        }
    }
}

void TransmissionMain::handleServerInitState()
{
    if (buffer.size() >= 24) {
        uint32_t nameLength = ((uint8_t)buffer[20] << 24) |
                             ((uint8_t)buffer[21] << 16) |
                             ((uint8_t)buffer[22] << 8) |
                             ((uint8_t)buffer[23]);

        if (buffer.size() >= 24 + nameLength) {
            std::cout << "\n--- Server Init ---" << std::endl;
            printHexDump(buffer, "ServerInit");

            uint16_t width = ((uint8_t)buffer[0] << 8) | (uint8_t)buffer[1];
            uint16_t height = ((uint8_t)buffer[2] << 8) | (uint8_t)buffer[3];

            std::cout << "\nFramebuffer: " << width << "x" << height << std::endl;

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

            std::cout << "Pixel Format:" << std::endl;
            std::cout << "  Bits per pixel: " << (int)bitsPerPixel << std::endl;
            std::cout << "  Depth: " << (int)depth << std::endl;
            std::cout << "  Big endian: " << (int)bigEndian << std::endl;
            std::cout << "  True color: " << (int)trueColor << std::endl;
            std::cout << "  RGB Max: R=" << redMax << " G=" << greenMax
                      << " B=" << blueMax << std::endl;
            std::cout << "  RGB Shift: R=" << (int)redShift << " G=" << (int)greenShift
                      << " B=" << (int)blueShift << std::endl;

            // Desktop name
            std::vector<char> name(buffer.begin() + 24, buffer.begin() + 24 + nameLength);
            std::cout << "Desktop name: \"";
            for (char c : name) {
                std::cout << (c >= 32 && c < 127 ? c : '?');
            }
            std::cout << "\"" << std::endl;

            buffer.clear();
            currentState = STATE_RUNNING;
            std::cout << "\n=== Handshake Complete! ===" << std::endl;
            std::cout << "Now in running state. You can send FramebufferUpdateRequest." << std::endl;
        }
    }
}

void TransmissionMain::handleRunningState()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    // Dump buffer after 100ms of no new data or when it reaches 16 bytes
    if (buffer.size() >= 16 ||
        (!buffer.empty() && (now - lastByteTime) > 100000000)) {  // 100ms in nanoseconds
        printHexDump(buffer, "Server Message");
        buffer.clear();
    }
}

void TransmissionMain::processVNCByte(int byte)
{
    lastByteTime = std::chrono::steady_clock::now().time_since_epoch().count();
    buffer.push_back((char)byte);

    // Real-time byte display
    std::cout << "[" << std::hex << std::setfill('0') << std::setw(2)
              << (int)(unsigned char)byte << std::dec << "] ";
    if (byte >= 32 && byte < 127) {
        std::cout << "'" << (char)byte << "' ";
    }
    std::cout << std::flush;

    switch (currentState) {
        case STATE_VERSION:
            handleVersionState();
            break;
        case STATE_SECURITY:
            handleSecurityState();
            break;
        case STATE_SECURITY_RESULT:
            handleSecurityResultState();
            break;
        case STATE_SERVER_INIT:
            handleServerInitState();
            break;
        case STATE_RUNNING:
            handleRunningState();
            break;
    }
}