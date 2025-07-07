#ifndef RELIABLE_CONNECTION_SERIAL1_H_
#define RELIABLE_CONNECTION_SERIAL1_H_

#include <Connection.h>
#include <ring_buffer.h>
#include <Arduino.h>

class ReliableConnectionSerial1 : public Connection
{
	public:
		static const char XON  = 0x11;
		static const char XOFF = 0x13;

		static const int maxBufferSize = 4096;
		static const int maxReadSize = 32;

		static ReliableConnectionSerial1* instance;

		static ReliableConnectionSerial1* getInstance();

		// Teensy 4.1 uses serialEvent1() for interrupt handling
		static void serialEvent1();

		~ReliableConnectionSerial1() {}

		void begin();
		void enableXonXoff();
		void disableXonXoff();

		// Connection interface
		int tryReadOne();
		char readOne();
		std::vector<char> read();
		std::vector<char> read(int size);
		void write(std::vector<char> bs);
		// end Connection

	private:
		bool xonXoffEnabled = false;
		FlowControlRingBuffer<char, maxBufferSize> ring;
		volatile bool paused = false;
		volatile bool buffer_full = false;

		ReliableConnectionSerial1();
};

#endif