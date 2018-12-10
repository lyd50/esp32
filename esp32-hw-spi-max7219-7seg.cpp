#include "esp8266-hw-spi-max7219-7seg.h"
#include <SPI.h>


#define DECODEMODE_ADDR 9
#define BRIGHTNESS_ADDR	10
#define SCANLIMIT_ADDR	11
#define SHUTDOWN_ADDR	12
#define DISPLAYTEST_ADDR 15
#define NOOP_ADDR		0


// init HW SPI
BgrMax7seg::BgrMax7seg(uint32_t spiFreq, int csPin, int dispAmount) {
	CS_PIN = csPin;
	_dispAmount = dispAmount;
	pinMode(CS_PIN, OUTPUT);
	digitalWrite(CS_PIN, HIGH);
	SPI.begin();
	SPI.beginTransaction(SPISettings(spiFreq, MSBFIRST, SPI_MODE0));
}

void BgrMax7seg::setBright(int brightness, int module) { // set brightness 
    if (brightness>=0 && brightness<16)	
        write(BRIGHTNESS_ADDR, brightness, module);
}

void BgrMax7seg::init() { //init all modules with scanlimit, TODO: remove scanlimit at all
	write(DISPLAYTEST_ADDR, 0, ALL_MODULES);	//turn off display test on all
	write(SCANLIMIT_ADDR, 7, ALL_MODULES);		//sanlimit to all 8 digits (=7)
	write(DECODEMODE_ADDR, 0, ALL_MODULES);		//turn off decode mode
	clear(ALL_MODULES);							//clear all modules
	write(SHUTDOWN_ADDR, 1, ALL_MODULES);		//turn all modules on
}

void BgrMax7seg::on(int module) {
	write(SHUTDOWN_ADDR, 0x01, module);
}

void BgrMax7seg::off(int module) {
	write(SHUTDOWN_ADDR, 0x00, module);
}

void BgrMax7seg::clear(int module) {
  for (int i = 1; i <=8; i++) {
	write(i, B00000000, module);
  }
}

void BgrMax7seg::table(byte address, int val, bool point, int module) {
	byte tableValue;
	tableValue = pgm_read_byte_near(segTable + val);
	if (point) {
		tableValue += 128;	//decimal point 
	}
	write(address, tableValue, module);
}

void BgrMax7seg::write(volatile byte address, volatile byte data, volatile int module) { //when module == 0 send data to all of them
	digitalWrite(CS_PIN, LOW);
	for (int i = _dispAmount; i >= 1; i--) {
		if ((module == 0) or (module == i)) {
			SPI.transfer(address);
			SPI.transfer(data);
		} else {
			SPI.transfer(NOOP_ADDR);
			SPI.transfer(NOOP_ADDR);
		}
	}
	digitalWrite(CS_PIN, HIGH);
}

void BgrMax7seg::print(String figure, int module) {
	figure.toUpperCase();			//we know only upperacase chars
	bool point = false;
	int len = figure.length();
	int position = len - 1;			//string are indexed from 0 => -1 and we start from the end

	for(int i = 1; i <= 8; i++) {	//
		if (position >= 0) {	
			// we are not outside  of the string
			if (figure[position] == '.') {
				if (position > 0) {
					position--;
					point = true;
				}
			} else {	//no point
				point = false;
			}
			if (figure[position] == ' ') {	//space is not in table
				write(i, point * 128, module);
			} else { 	//search the table
				int parseInt = figure[position] - '-';	// first character of the segTable needs to be here
				table(i, parseInt, point, module);
			}
			position--;
		} else {
		// will print space
			write(i, 0, module);
		}		

	}
}
