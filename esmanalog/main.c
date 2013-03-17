#ifndef F_CPU
#error F_CPU not defined
#endif

#include <string.h>
#include <avr/io.h>
#include <util/delay.h>

#include "timer0.h"
#include "board.h"

void setup(void)
{
    /*
     * PD2 output
     * PB3 indicator led
     * PB5 indicator led
     */

    DDRB = _BV(PB2) | _BV(PB3) |_BV(PB5);
    PORTB = ~(_BV(PB2) | _BV(PB3) | _BV(PB5)); /* enable pull-ups */

    /*
     * Set ADC prescaler /128, 16 Mhz / 128 = 125 KHz which is inside
     * the desired 50-200 KHz range.
     */

    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}

int analog_read(int chan)
{
    uint8_t low, high;

    /* AVCC with external capacitor at AREF pin, select channel */
    ADMUX = (_BV(REFS0) | (chan & 0x0f));

    ADCSRA |= _BV(ADSC); /* start single conversion */

    /* wait for conversion to complete */
    while (ADCSRA & _BV(ADSC))
	;

    low  = ADCL;
    high = ADCH;

    return (high << 8) | low;
}

int main(void)
{
    unsigned long t = 0, t0 = 0;

    setup();
    setup_timer0();

    for (;;) {
	if ((t = millis()) - t0 > 500) {
	    PORTB ^= _BV(PB5);
	    t0 = t;

	    int pot = analog_read(0);
	    int ldr = analog_read(1);

	    if (ldr < pot) {
		PORTB |= _BV(PB3);
	    } else {
		PORTB &= ~_BV(PB3);
	    }

#ifdef WIGGLE_PB2	    
	    for (int i = 0; i < ldr; i++) {
		PORTB ^= _BV(PB2);
	    }
#endif
	}
    }

    return 0;
}
