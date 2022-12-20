/*
 * Manage control of Crystalfontz OLED P/N CFAL25664C0-021M-[W or Y]
 * Uses the Adafruit GFX Font Library directly. An explanation of the font file syntax is here: https://learn.adafruit.com/creating-custom-symbol-font-for-adafruit-gfx-library/understanding-the-font-specification
 * First/main section of font file contains a large binary bitmap, where the bytes are expressed in hex
 * The latter section of the file is essentially a ciper which describes the location of each glph in the bitmap
 * 	1: index (starting point) in bitmap array
 * 	2: width of glyph
 * 	3: height of glyph 
 * 	4: when drawing glyph, pixels to next character. A monospaced font would have this be always the same.
 * 	5: dx, horizontal centering w.r.t. baseline
 * 	6: dY, vertical centering w.r.t. baseline
 *
 * J. Haupt
 *
 * */

#include <Arduino.h>
#include "OLED.h"

OLED::OLED(){	//The constructor
}

void OLED::init(){
	pinMode(EN_12V, OUTPUT); //configure 12v enable pin
	DDRA |= bit(0);	//set port A0 as output
	DDRC |= bit(0);	//set port C0 as output
	DDRC |= bit(1);	//set port C1 as output
	
	delay(100);
	SET_CS;
	SPI.begin();
	SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

	CLR_RESET;
	delay(100);
	SET_RESET;

	_writeCommand(0Xfd); //Set Command Lock
	_writeCommand(0X12); //(12H=Unlock,16H=Lock)
	_writeCommand(0XAE); //Display OFF(Sleep Mode)

	_setColAddr(0x00, 0x7f);
	_setRowAddr(0x00, 0x3f);

	_writeCommand(0X81); //Set contrast
	_writeCommand(0x2f);

	_writeCommand(0Xa0); //Set Remap
	_writeCommand(0Xc3);

	_writeCommand(0Xa1); //Set Display Start Line
	_writeCommand(0X00);

	_writeCommand(0Xa2); //Set Display Offset
	_writeCommand(0X00);

	_writeCommand(0Xa4); //Normal Display

	_writeCommand(0Xa8); //Set Multiplex Ratio
	_writeCommand(0X3f);

	_writeCommand(0Xab); //Set VDD regulator
	_writeCommand(0X01); //Regulator Enable

	_writeCommand(0Xad); //External /Internal IREF Selection
	_writeCommand(0X8E);

	_writeCommand(0Xb1); //Set Phase Length
	_writeCommand(0X22);

	_writeCommand(0Xb3); //Display clock Divider
	_writeCommand(0Xa0);

	_writeCommand(0Xb6); //Set Second precharge Period
	_writeCommand(0X04);

	_writeCommand(0Xb9); //Set Linear LUT

	_writeCommand(0Xbc); //Set pre-charge voltage level
	_writeCommand(0X10); //0.5*Vcc

	_writeCommand(0Xbd); //Pre-charge voltage capacitor Selection
	_writeCommand(0X01);

	_writeCommand(0Xbe); //Set cOM deselect voltage level
	_writeCommand(0X07); //0.82*Vcc

	clear();				// Clear Screen
	_writeCommand(0Xaf); //Display ON
}

void OLED::_drawChar(int bytestart, int gwidth, int gheight, int cursX, int cursY){
	int byteind;
	int bitcnt = 8; //counts what bit we're on in the current byte

	byteind = bytestart;	//Set our byte index to the starting byte of this glyph
	for (int yi = 0; yi < gheight; yi++){	//loop over each row on display
		_setColAddr(cursX, cursX+gwidth);				// Set starting/ending x address on OLED
		_setRowAddr(cursY+yi, cursY+gheight);				//  Set starting/ending y address on OLED 0, 63
		for (int xi = 0; xi < gwidth; xi++){	//loop over pixels in current row on display
			bitcnt--;
			_writeData(  255*bitRead(pgm_read_byte(&FreeSans12pt7bBitmaps[byteind]),bitcnt)  );
			if (bitcnt == 0){
				byteind++;
				bitcnt = 8;
			}
		}
	}
}

void OLED::_drawCharHalf(int bytestart, int gwidth, int gheight, int cursX){ //!!!EXPERIMENTAL, NOT WORKING
	int byteind;
	int bitcnt = 8; //counts what bit we're on in the current byte

	byteind = bytestart;	//Set our byte index to the starting byte of this glyph
	for (int yi = 0; yi < (gheight/2); yi++){	//loop over each row on display
		_setColAddr(cursX, cursX+(gwidth/2));				// Set starting/ending x address on OLED
		_setRowAddr(yi+20, gheight+20);				//  Set starting/ending y address on OLED 0, 63
		for (int xi = 0; xi < (gwidth/2); xi=xi+2){	//loop over pixels in current row on display
			bitcnt = bitcnt - 2;
			_writeData(  255*bitRead(pgm_read_byte(&FreeSans24pt7bBitmaps[byteind]),bitcnt)  );
			if (bitcnt == 0){
				byteind++;
				bitcnt = 8;
			}
		}
	}
}

void OLED::print(char str[10], int cursX, int cursY){
	_wake();
	int bytestart, gwidth, gheight;
	for (int i = 0; i < strlen(str);i++){

		//Get glyph's starting index in the bitmap, as well as height and width
		bytestart = pgm_read_word(&FreeSans12pt7bGlyphs[str[i]-32].bitmapOffset); 
		gwidth = pgm_read_word(&FreeSans12pt7bGlyphs[str[i]-32].width);
		gheight = pgm_read_word(&FreeSans12pt7bGlyphs[str[i]-32].height);
		
		_drawChar(bytestart, gwidth, gheight, cursX, cursY);	//Draw the glyph
		
		cursX = cursX + pgm_read_byte(&FreeSans12pt7bGlyphs[str[i]-32].xAdvance); //Advance the cursor for the next pass
	}
}

void OLED::displayFromFlash(){	
	unsigned char i, j;

	for (i = 0; i < 64; i++){
		_setColAddr(0x00,0x7f);	   // Set higher & lower column Address
		_setRowAddr(i, 0x3f);				   //  Set Row-Address 

		for (j = 0; j < 128; j++){
			//wrap the address in a pgm_read so we can get to the data
			_writeData(pgm_read_byte(&Grey_2BPP[i * 128 + j]));
		}
  	}
}

void OLED::clear(){
	unsigned char i, j;

	for (i = 0; i < 64; i++){
		_setColAddr(0x00, 0x7f);	   // Set higher & lower column Address
		_setRowAddr(i, 0x3f);				   //  Set Row-Address 

		for (j = 0; j < 128; j++){
			_writeData(0x00);
		}
	}
	digitalWrite(EN_12V, LOW);	//sleep
	_oledActive = false;
}

void OLED::autoClear(uint32_t odelay){
	if (_oledActive == true){
		_stopwatch++;
		if (_stopwatch > odelay){
			_stopwatch = 0;
			clear();
		}
	}
}

void OLED::_wake(){
	digitalWrite(EN_12V, HIGH);
	_oledActive = true;
	_stopwatch = 0;
}

void OLED::_writeCommand(uint8_t command){
	// Select the LCD controller
	CLR_CS;
	// Select the LCD's command register
	CLR_DC;

	//Send the command via SPI:
	SPI.transfer(command);
	//deselect the controller
	SET_CS;
}

void OLED::_writeData(uint8_t data){
	//Select the LCD's data register
	SET_DC;
	//Select the LCD controller
	CLR_CS;
	//Send the command via SPI:
	SPI.transfer(data);

	// Deselect the LCD controller
	SET_CS;
}

void OLED::_setColAddr(uint8_t start, uint8_t end){
	_writeCommand(0x15);
	_writeCommand(start);		    // Set start column address
	_writeCommand(end);			    // Set end column address
}

void OLED::_setRowAddr(uint8_t start, uint8_t end){
	_writeCommand(0x75);
	_writeCommand(start);		    // Set start row address
	_writeCommand(end);			    // Set end row address
}

void OLED::_setStartLine(uint8_t line){
	_writeCommand(0x40 | line);	// Set  Display start line
}

void OLED::_setContrast(uint8_t contrast){
	_writeCommand(0x81);			    // Set contrast control
	_writeCommand(contrast);	    //   Default => 0x80 (Maximum)
}

