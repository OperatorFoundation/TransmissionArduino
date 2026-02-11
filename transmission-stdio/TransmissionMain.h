// TransmissionMain.h
#ifndef MAIN_H
#define MAIN_H

#include <ReliableConnectionTcpMacOS.h>
#include <vector>
#include <string>

enum VNCState {
  STATE_VERSION,
  STATE_SECURITY,
  STATE_SECURITY_RESULT,
  STATE_SERVER_INIT,
  STATE_RUNNING
};

class TransmissionMain
{
  public:
    TransmissionMain(const std::string& host, uint16_t port);
    ~TransmissionMain();

    bool connect();
    void loop();

  private:
    ReliableConnectionTcpMacOS vnc;
    VNCState currentState;
    std::vector<char> buffer;
    unsigned long lastByteTime;

    void processVNCByte(int byte);
    void printHexDump(const std::vector<char>& data, const char* label);
    void printAsciiString(const std::vector<char>& data);
    void handleVersionState();
    void handleSecurityState();
    void handleSecurityResultState();
    void handleServerInitState();
    void handleRunningState();
};

#endif //MAIN_H