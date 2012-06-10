# Name: Makefile
# Author: <insert your name here>
# Copyright: <insert your copyright message here>
# License: <insert your license reference here>

# This is a prototype Makefile. Modify it according to your needs.
# You should at least check the settings for
# DEVICE ....... The AVR device you compile for
# CLOCK ........ Target AVR clock rate in Hertz
# OBJECTS ...... The object files created from your source files. This list is
#                usually the same as the list of source files with suffix ".o".
# PROGRAMMER ... Options to avrdude which define the hardware you use for
#                uploading to the AVR and the interface where this hardware
#                is connected.
# FUSES ........ Parameters for avrdude to flash the fuses appropriately.

include ../../lib/mk/fuses.mk

DEVICE     = attiny4313
#CLOCK      = 8000000
CLOCK      = 16000000
#FUSES      = $(FUSES_4313_AT8)
FUSES      = $(FUSES_4313_EXT)

PROGRAMMER = -c avrisp2 -P usb
OBJECTS    = larson.o timer0.o irrecv.o

# Tune the lines below only if you know what you are doing:

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -std=gnu99 -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# symbolic targets:
all:	larson.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:larson.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

# Xcode uses the Makefile targets "", "clean" and "install"
install: flash fuse

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID larson.hex

clean:
	/bin/rm -f larson.hex larson.elf larson.map $(OBJECTS) *~

# file targets:
larson.elf: $(OBJECTS)
	$(COMPILE) -o larson.elf -Wl,-Map=larson.map  $(OBJECTS)

larson.hex: larson.elf
	/bin/rm -f larson.hex
	avr-objcopy -j .text -j .data -O ihex larson.elf larson.hex
	avr-size -t $(OBJECTS)
	avr-size larson.elf

# If you have an EEPROM section, you must also create a hex file for the
# EEPROM and add it to the "flash" target.

# Targets for code debugging and analysis:
disasm:	larson.elf
	avr-objdump -d larson.elf

cpp:
	$(COMPILE) -E larson.c

# DO NOT DELETE

larson.s: larson.c

irrecv.o: irrecv.h irrecvint.h
larson.o: board.h timer0.h irrecv.h irremote.h
timer0.o: timer0.h
