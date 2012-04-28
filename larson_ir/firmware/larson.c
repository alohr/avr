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

#include "board.h"
#include "timer0.h"
#include "irrecv.h"
#include "irremote.h"

#define shortdelay(); asm("nop\n\t"	     \
"nop\n\t");


uint16_t eepromWord __attribute__((section(".eeprom")));

int main (void)
{
    uint8_t LEDs[9]; // Storage for current LED values

    int8_t eyeLoc[5]; // List of which LED has each role, leading edge through tail.

    uint8_t LEDBright[4] = {1U,4U,2U,1U};   // Relative brightness of scanning eye positions

    int8_t j, m;

    uint8_t position, direction;

    uint8_t prescale = 0;	// Prescale timer for  for robot mode

    uint8_t ILED, RLED, MLED;

    uint8_t delaytime;

    uint8_t skinnyEye = 1;
    uint8_t pt, speedLevel;
    uint8_t 	UpdateConfig;
    uint8_t BrightMode;
    uint8_t modeswitched;

    uint8_t CycleCountLow;
    uint8_t LED0 = 0;
    uint8_t LED1 = 0;
    uint8_t LED2 = 0;
    uint8_t LED3 = 0;
    uint8_t LED4 = 0;
    uint8_t LED5 = 0;
    uint8_t LED6 = 0;
    uint8_t LED7 = 0;
    uint8_t LED8 = 0;

    uint8_t robotmode = 0;

    unsigned long t0, t1, tmax = 20000;
    decode_results irresults = { .value = 0 };

#if 0

//Initialization routine: Clear watchdog timer-- this can prevent several things from going wrong.
    MCUSR &= 0xF7;		//Clear WDRF Flag
    WDTCSR	= 0x18;		//Set stupid bits so we can clear timer...
    WDTCSR	= 0x00;
#endif

//Data direction register: DDR's
//Port A: 0, 1 are inputs.
//Port B: 0-3 are outputs, B4 is an input.
//Port D: 1-6 are outputs, D0 is an input.

    DDRB = 0x0f;
    DDRD = 126U;

    DDRB |= _BV(PB5);

    PORTD = 0;

   

    // Visualize outputs:
    //
    // L to R:
    //
    // D2 D3 D4 D5 D6 B0 B1 B2 B3

    delaytime = 0;

    direction = 0;
    position = 0;
    speedLevel = 2;  // Range: 1, 2, 3
    BrightMode = 1;
    CycleCountLow = 0;
    UpdateConfig = 0;
    modeswitched = 0;


    if (skinnyEye) {
	LEDBright[0] = 0;
	LEDBright[1] = 4;
	LEDBright[2] = 1;
	LEDBright[3] = 0;
    }


    //Check EEPROM values:

    pt = (uint8_t) (eeprom_read_word(&eepromWord)) ;
    speedLevel = pt >> 4;
    BrightMode = pt & 1;

    if (pt == 0xFF)
    {
	BrightMode = 0;
    }


    if (speedLevel > 3)
	speedLevel = 1;

    if ((speedLevel == 2) || (speedLevel == 3)) {
	delaytime = 0;
    }
    else
    {   speedLevel = 1;
	delaytime = 1;
    }

    setup_timer0();
    setup_irrecv();

    t0 = micros();

    BrightMode = 1;

    // main loop
    for (;;) {

	if (irrecv_decode(&irresults)) {
	    if (irresults.value != REPEAT) {
		switch (irresults.value) {
		case CHANNEL_1:
		    PORTB ^= _BV(PB5);
		    break;
		}
	    }
	    irrecv_resume();
	}

	if ((t1 = micros()) - t0 > tmax) {
	    t0 = micros();


#if 0	    
	    CycleCountLow++;
	    if (CycleCountLow > 250)
		CycleCountLow = 0;

	    if (UpdateConfig) {
		// Need to save configuration byte to EEPROM
		if (CycleCountLow > 100) {
		    // Avoid burning EEPROM in event of flaky power connection resets

		    UpdateConfig = 0;
		    pt = (speedLevel << 4) | (BrightMode & 1);
		    eeprom_write_word(&eepromWord, (uint8_t) pt);
		    // Note: this function causes a momentary
		    // brightness glitch while it writes the EEPROM.
		    // We separate out this section to minimize the effect.
		}
	    }
#endif

	    // Check for button press
	    if (robotmode) {
		prescale++;
		if (speedLevel == 3)
		    prescale++;

		if (prescale >= 11) {	// Fine tune the step time with this factor.
		    position++;
		    prescale = 0;
		}

		if (position > 18)
		    position = 0;

		// Direction: LEDs chase DOWN TOWARDS BATTERY CONNECTIONS.
		LED8 = 30 * ((position >> 1) == 0);
		LED7 = 30 * ((position >> 1) == 1);
		LED6 = 30 * ((position >> 1) == 2);
		LED5 = 30 * ((position >> 1) == 3);
		LED4 = 30 * ((position >> 1) == 4);
		LED3 = 30 * ((position >> 1) == 5);
		LED2 = 30 * ((position >> 1) == 6);
		LED1 = 30 * ((position >> 1) == 7);
		LED0 = 30 * ((position >> 1) == 8);

	    } else {
		position++;

		if (speedLevel == 3)
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

