DEVICE     = attiny2313
CLOCK      = 1000000
PROGRAMMER = -c avrisp2 -P usb
FUSES      = -U lfuse:w:0x64:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

OBJECTS    = main.o

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -Wall -std=gnu99 -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# symbolic targets:
all:	main.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

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
	/bin/rm -f main.hex main.elf $(OBJECTS) *~

# file targets:
main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

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
