#!/bin/bash
#To flash USB bridge firmware to ATmega16U2 (or 8U2)
#Connect AVR-ISP MkII to J17 ICSP port and run this file from within this directory.

avrdude -p at90usb82 -F -P usb -c avrispmkii -U flash:w:MEGA-dfu_and_usbserial_combined.hex -U lfuse:w:0xFF:m -U hfuse:w:0xD9:m -U efuse:w:0xF4:m -U lock:w:0x0F:m
