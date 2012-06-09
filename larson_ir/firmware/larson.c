/*
larson.c
The Larson Scanner

Written by Windell Oskay, http://www.evilmadscientist.com/

 Copyright 2011 Windell H. Oskay
 Distributed under the terms of the GNU General Public License, please see below.

  An avr-gcc program for the Atmel ATTiny2313

 Version 1.4   Last Modified:  8/30/2011.
 Written for Evil Mad Science Larson Scanner Kit, based on the "ix" circuit board.

 Larson Scanner docs are at: http://wiki.evilmadscience.com/Larson_Scanner
 This code hosted at http://code.google.com/p/larsonscanner/

 Improvements in v 1.4:
 * Unidirectional chaser mode added, "robotmode" for making a Robots tie.
 Activate by soldering Opt 2. on the circuit board, or by holding button down for about 10 s.
 *Corrected bug in earlier version-- skinny eye mode was at opt 2, not opt 1.

 Improvements in v 1.3:
 * EEPROM is used to *correctly* remember last speed & brightness mode.

 Improvements in v 1.2:
 * Skinny "eye" mode.  Hold button at turn-on to try this mode.  To make it default,
 solder jumper "Opt 1."  (If skinny mode is default, holding button will disable it temporarily.)
 * EEPROM is used to remember last speed & brightness mode.


 More information about this project is at
 http://wiki.evilmadscience.com/Larson_Scanner



 -------------------------------------------------
 USAGE: How to compile and install



 A makefile is provided to compile and install this program using AVR-GCC and avrdude.

 To use it, follow these steps:
 1. Update the header of the makefile as needed to reflect the type of AVR programmer that you use.
 2. Open a terminal window and move into the directory with this file and the makefile.
 3. At the terminal enter
 make clean   <return>
 make all     <return>
 make install <return>
 4. Make sure that avrdude does not report any errors.  If all goes well, the last few lines output by avrdude
 should look something like this:

 avrdude: verifying ...
 avrdude: XXXX bytes of flash verified

 avrdude: safemode: lfuse reads as 62
 avrdude: safemode: hfuse reads as DF
 avrdude: safemode: efuse reads as FF
 avrdude: safemode: Fuses OK

 avrdude done.  Thank you.


 If you a different programming environment, make sure that you copy over the fuse settings from the makefile.


 -------------------------------------------------

 This code should be relatively straightforward, so not much documentation is provided.  If you'd like to ask
 questions, suggest improvements, or report success, please use the evilmadscientist forum:
 http://www.evilmadscientist.com/forum/


 -------------------------------------------------


*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "board.h"
#include "timer0.h"
#include "irrecv.h"
#include "irremote.h"

#define shortdelay(); asm("nop\n\tnop\n\t");

enum {
    MIN_DELAY     = 2000,
    MAX_DELAY     = 30000,
    DEFAULT_DELAY = 20000,
    DELAY_STEP    = 1000,
};

uint16_t eepromWord __attribute__((section(".eeprom")));

union {
    uint16_t data;
    struct {
	int brightMode : 1;
	int skinnyEye  : 1;
	int delaytime  : 14;
    } s;
} config;

unsigned long t0, t1, delay_time = DEFAULT_DELAY;
unsigned long last_update_time;

decode_results irresults = { .value = 0 };
uint8_t skinnyEye = 0;
uint8_t LEDBright[4] = {1U,4U,2U,1U};   // Relative brightness of scanning eye positions


void toggle_skinnyEye(void)
{
    skinnyEye = !skinnyEye;

    if (skinnyEye) {
	LEDBright[0] = 0;
	LEDBright[1] = 4;
	LEDBright[2] = 1;
	LEDBright[3] = 0;
    } else {
	LEDBright[0] = 1;
	LEDBright[1] = 4;
	LEDBright[2] = 2;
	LEDBright[3] = 1;
    }
}

int main (void)
{
    uint8_t LEDs[9]; // Storage for current LED values

    int8_t eyeLoc[5]; // List of which LED has each role, leading edge through tail.

    int8_t j, m;

    uint8_t position, direction;

    uint8_t ILED, RLED, MLED;

    uint8_t pt;
    uint8_t BrightMode = 1;
    uint8_t UpdateConfig = 0;
    uint8_t CycleCountLow = 0;

    uint8_t LED0 = 0;
    uint8_t LED1 = 0;
    uint8_t LED2 = 0;
    uint8_t LED3 = 0;
    uint8_t LED4 = 0;
    uint8_t LED5 = 0;
    uint8_t LED6 = 0;
    uint8_t LED7 = 0;
    uint8_t LED8 = 0;

    // Data direction register: DDR's
    // Port A: 0, 1 are inputs.
    // Port B: 0-3 are outputs, B4 is an input.
    // Port D: 1-6 are outputs, D0 is an input.

    DDRB = 0x0f;
    DDRD = 0x7e;

    // Visualize outputs:
    //
    // L to R:
    //
    // D2 D3 D4 D5 D6 B0 B1 B2 B3

    // Check EEPROM values
    /* config.data = eeprom_read_word(&eepromWord); */
    /* BrightMode = config.s.brightMode; */
    /* skinnyEye = config.s.skinnyEye; */
    /* delay_time = config.s.delaytime; */

    direction = 0;
    position = 0;

    setup_timer0();
    setup_irrecv();

    t0 = micros();

    // main loop
    for (;;) {

	if (irrecv_decode(&irresults)) {

	    switch (irresults.value & 0xff) {
	    case VOLUME_UP:
		if (delay_time > MIN_DELAY)
		    delay_time -= DELAY_STEP;
		break;
	    case VOLUME_DOWN:
		if (delay_time < MAX_DELAY)
		    delay_time += DELAY_STEP;
		break;
	    case MUTE:
		if ((t1 = micros()) - last_update_time > 500000UL) {
		    last_update_time = t1;
		    BrightMode = !BrightMode;
		    UpdateConfig = 1;
		}
		break;
	    case TV_AV:
		if ((t1 = micros()) - last_update_time > 500000UL) {
		    last_update_time = t1;
		    toggle_skinnyEye();
		    UpdateConfig = 1;
		}
		break;
	    }
	    irrecv_resume();
	}

	if ((t1 = micros()) - t0 > delay_time) {
	    t0 = t1;

	    if (++CycleCountLow > 250)
		CycleCountLow = 0;

	    if (UpdateConfig) {
		// Need to save configuration byte to EEPROM
		if (CycleCountLow > 100) {
		    // Avoid burning EEPROM in event of flaky power connection resets
		    UpdateConfig = 0;
		    config.s.brightMode = BrightMode;
		    config.s.skinnyEye = skinnyEye;
		    config.s.delaytime = delay_time;

		    eeprom_write_word(&eepromWord, config.data);
		    // Note: this function causes a momentary brightness glitch while it
		    // writes the EEPROM.
		    // We separate out this section to minimize the effect.

		    {
			uint8_t i = 0;
			for (i = 0; i < 5; i++) {
			    PORTB |= _BV(PB5);
			    _delay_ms(100);
			    PORTB &= ~(_BV(PB5));
			}
		    }

		}
	    }

	    position++;

	    if (position >= 128) {
		position = 0;
		direction = !direction;
	    }

	    if (direction == 0)  // Moving to right, as viewed from front.
	    {
		ILED = (15+position) >> 4;
		RLED = (15+position) - (ILED << 4);
		MLED = 15 - RLED;
	    }
	    else
	    {
		ILED = (127 - position) >> 4;
		MLED = (127 - position)  - (ILED << 4);
		RLED =  15 - MLED;
	    }

	    j = 0;
	    while (j < 9) {
		LEDs[j] = 0;
		j++;
	    }

	    j = 0;
	    while (j < 5) {
		if (direction == 0)
		    m = ILED + (2 - j);	// e.g., eyeLoc[0] = ILED + 2;
		else
		    m = ILED + (j - 2);  // e.g., eyeLoc[0] = ILED - 2;

		if (m > 8)
		    m -= (2 * (m - 8));

		if (m < 0)
		    m *= -1;

		eyeLoc[j] = m;

		j++;
	    }

	    j = 0;		// For each of the eye parts...
	    while (j < 4) {
		LEDs[eyeLoc[j]]   += LEDBright[j]*RLED;
		LEDs[eyeLoc[j+1]] += LEDBright[j]*MLED;
		j++;
	    }
	    LED0 = LEDs[0];
	    LED1 = LEDs[1];
	    LED2 = LEDs[2];
	    LED3 = LEDs[3];
	    LED4 = LEDs[4];
	    LED5 = LEDs[5];
	    LED6 = LEDs[6];
	    LED7 = LEDs[7];
	    LED8 = LEDs[8];
	}

	if (BrightMode == 0) {	
	    j = 0;
	    while (j < 60) {
		// Truncate brightness at a max value (60) in the interest of speed.
		uint8_t portb, portd;

		portd = PORTD & 0x83;
		if (LED0 > j)
		    PORTD = portd | _BV(PD2);
		else
		    PORTD = portd & ~(_BV(PD2));

		if (LED1 > j)
		    PORTD = portd | _BV(PD3);
		else
		    PORTD = portd & ~(_BV(PD3));

		if (LED2 > j)
		    PORTD = portd | _BV(PD4);
		else
		    PORTD = portd & ~(_BV(PD4));

		if (LED3 > j)
		    PORTD = portd | _BV(PD5);
		else
		    PORTD = portd & ~(_BV(PD5));

		if (LED4 > j)
		    PORTD = portd | _BV(PD6);
		else
		    PORTD = portd & ~(_BV(PD6));

		portb = PORTB & 0xf0;
		if (LED5 > j) {
		    PORTB = portb | _BV(PB0);
		    PORTD = portd;
		} else {
		    PORTB = portb;
		    PORTD = portd;
		}

		if (LED6 > j)
		    PORTB = portb | _BV(PB1);
		else
		    PORTB = portb & ~(_BV(PB1));

		if (LED7 > j)
		    PORTB = portb | _BV(PB2);
		else
		    PORTB = portb & ~(_BV(PB2));

		if (LED8 > j)
		    PORTB = portb | _BV(PB3);
		else
		    PORTB = portb & ~(_BV(PB3));

		j++;
		asm("nop");	 // Delay to make up time difference versus branch.
		asm("nop");
		asm("nop");
		PORTB = portb;
	    }
	}
	else {
	    // Full power routine
	    j = 0;
	    while (j < 70) {
		pt = 0;
		if (LED0 > j)
		    pt = _BV(PD2);
		if (LED1 > j)
		    pt |= _BV(PD3);
		if (LED2 > j)
		    pt |= _BV(PD4);
		if (LED3 > j)
		    pt |= _BV(PD5);
		if (LED4 > j)
		    pt |= _BV(PD6);

		PORTD = (PORTD & 0x83) | pt;
		shortdelay();

		pt = 0;
		if (LED5 > j)
		    pt |= _BV(PB0);
		if (LED6 > j)
		    pt |= _BV(PB1);
		if (LED7 > j)
		    pt |= _BV(PB2);
		if (LED8 > j) 
		    pt |= _BV(PB3);
		
		PORTB = (PORTB & 0xe0) | pt;
		shortdelay();
		j++;
	    }
	}

    }	//End main loop
    return 0;
}

