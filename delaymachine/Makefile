include ../lib/mk/fuses.mk

DEVICE     = atmega328p
#CLOCK      = 16000000
#FUSES 	   = $(FUSES_M328P_EXT)

CLOCK      = 8000000
FUSES 	   = $(FUSES_M328P_AT8)

PROGRAMMER = -c avrisp2 -P usb

OBJECTS    = main.o timer0.o

# Tune the lines below only if you know what you are doing:

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -std=gnu99 -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# symbolic targets:
all:	main.hex

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
	$(AVRDUDE) -U flash:w:main.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

# Xcode uses the Makefile targets "", "clean" and "install"
install: flash fuse

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID main.hex

clean:
	/bin/rm -f main.hex main.elf main.map $(OBJECTS) *~

# file targets:
main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf -Wl,-Map=main.map  $(OBJECTS)

main.hex: main.elf
	/bin/rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size -t $(OBJECTS)
	avr-size main.elf

# If you have an EEPROM section, you must also create a hex file for the
# EEPROM and add it to the "flash" target.

# Targets for code debugging and analysis:
disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E main.c

# DO NOT DELETE
