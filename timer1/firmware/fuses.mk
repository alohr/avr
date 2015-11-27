FUSES_attiny2313_1000000	= -U lfuse:w:0x64:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
FUSES_attiny2313_8000000	= -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
FUSES_attiny2313_16000000	= -U lfuse:w:0xce:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

FUSES_attiny4313_1000000	= -U lfuse:w:0x64:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
FUSES_attiny4313_8000000	= -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
FUSES_attiny4313_16000000	= -U lfuse:w:0xde:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

FUSES_attiny861_1000000		= -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

FUSES_attiny85_1000000		= -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
FUSES_attiny85_8000000		= -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
FUSES_attiny85_16000000		= -U lfuse:w:0xef:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
FUSES_attiny85_16000000_BOD43	= -U lfuse:w:0xef:m -U hfuse:w:0xdc:m -U efuse:w:0xff:m

FUSES_atmega8_1000000		= -U lfuse:w:0xe1:m -U hfuse:w:0xd9:m
FUSES_atmega8_8000000		= -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m
FUSES_atmega8_16000000		= -U lfuse:w:0xdf:m -U hfuse:w:0xca:m

FUSES_atmega328p_1000000	= -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m
FUSES_atmega328p_8000000	= -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m
FUSES_atmega328p_16000000	= -U lfuse:w:0xff:m -U hfuse:w:0xda:m -U efuse:w:0x05:m

FUSES = $(FUSES_$(DEVICE)_$(CLOCK))
