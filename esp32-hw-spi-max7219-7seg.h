#ifndef esp32-hw-spi-max7219-7seg_h
#define esp32-hw-spi-max7219-7seg_h

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#define ALL_MODULES		0	//use as module number to send to all

const static byte segTable [] PROGMEM = { // starting from 0
	1,		// -
	128,	// .
	0,		// /
    126, 	// 0
	48,		// 1
	109,	// 2
	121,	// 3
	51,		// 4
	91,		// 5
	95,		// 6
	112,	// 7
	127,	// 8
	123,	// 9
	0, 0, 0, 0, 0, 0, 0,	// : ; < = > ? @ - will not display these
	119, 31, 78, 61, 79, 71, 95, 23, 48, 60, 87, 14, 84, 21, 126, 103, 115, 5, 91, 15, 62, 62, 42, 55, 59, 109	// A .. Z
};

class BgrMax7seg
{
	private:
		int CS_PIN;
		int _digitLimit;
		int _dispAmount;
		void table(byte address, int val, bool point, int module);	
	public:
		BgrMax7seg(uint32_t spiFreq, int csPin, int dispAmount = 1);	//dispAmount = number of connected modules
		void setBright(int brightness, int module = 1);
		void init();
		void print(String figure, int module = 1);
		void write(byte address, byte data, int module = 1);
		void clear(int module = 1);
		void on(int module = 1);
		void off(int module = 1);		
};

#endif	//esp8266-hw-spi-max7219-7seg.h
