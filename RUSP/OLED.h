#ifndef OLED_H
#define OLED_H
#include <Arduino.h>
#include <SPI.h>
#include "splash.h"
#include "gfxfont.h"
#include "./Fonts/FreeSans12pt7b.h"	//Must be within project directory, not the one in the Adafruit-GFX library
#include "./Fonts/FreeSans24pt7b.h"	//Must be within project directory, not the one in the Adafruit-GFX library

//non-data SPI pins
#define CLR_CS     (PORTA &= ~bit(0)) //set digital 22 (PA0) low
#define SET_CS     (PORTA |=  bit(0)) //set digital 22 (PA0) high
#define CLR_RESET  (PORTC &= ~bit(0)) //set digital 37 (PC0) low
#define SET_RESET  (PORTC |=  bit(0)) //set digital 37 (PC0) high
#define CLR_DC     (PORTC &= ~bit(1)) //set digital 36 (PC1) low
#define SET_DC     (PORTC |=  bit(1)) //set digital 36 (PC1) high

//12V enable pin
#define EN_12V 33

// Define some constants
#define XLevelL		0x00
#define XLevelH		0x10
#define XLevel		((XLevelH&0x0F)*16+XLevelL)
#define HRES		128
#define VRES		56
#define	Brightness	0xBF

class OLED {
	public:
		OLED();
		void init();
		void displayFromFlash();
		void print(char str[10], int cursX, int cursY);
		void clear();
		void autoClear(uint32_t odelay);

	private:
		uint32_t _stopwatch = 0;
		bool _oledActive = false;
		void _wake();
		void _drawChar(int bytestart, int gwidth, int gheight, int cursX, int cursY);
		void _drawCharHalf(int bytestart, int gwidth, int gheight, int cursX);
		void _writeCommand(uint8_t command);
		void _writeData(uint8_t data);
		void _setColAddr(uint8_t start, uint8_t end);
		void _setRowAddr(uint8_t start, uint8_t end);
		void _setStartLine(uint8_t line);
		void _setContrast(uint8_t contrast);
};

#endif
