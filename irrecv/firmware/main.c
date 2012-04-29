#ifndef F_CPU
#error F_CPU not defined
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "irrecv.h"
#include "state.h"


void process(ledstate *state, const decode_results *r);

void led(int8_t i)
{
    uint8_t portd = PORTD & 0x83;
    uint8_t portb = PORTB & 0xd0;

    switch (i) {
    case 0:
	PORTD = portd | _BV(PD2);
	PORTB = portb;
	break;
    case 1:
	PORTD = portd | _BV(PD3);
	PORTB = portb;
	break;
    case 2:
	PORTD = portd | _BV(PD4);
	PORTB = portb;
	break;
    case 3:
	PORTD = portd | _BV(PD5);
	PORTB = portb;
	break;
    case 4:
	PORTD = portd | _BV(PD6);
	PORTB = portb;
	break;
    case 5:
	PORTD = portd;
	PORTB = portb | _BV(PB0);
	break;
    case 6:
	PORTD = portd;
	PORTB = portb | _BV(PB1);
	break;
    case 7:
	PORTD = portd;
	PORTB = portb | _BV(PB2);
	break;
    case 8:
	PORTD = portd;
	PORTB = portb | _BV(PB3);
	break;
    default:
	PORTD = portd;
	PORTB = portb;
    }
}


int main(void)
{
    decode_results r = { .value = 0 };
    ledstate state = { .on = 0, .led = 0, .toggle = 0xff };

    /*
     *         7 6 5 4  3 2 1 0
     *  PORTD  0 1 1 1  1 1 0 0 = 0x7c (~ 0x83)
     *  PORTB  0 0 1 0  1 1 1 1 = 0x2f (~ 0xd0)
     *
     */

    DDRD = 0x7c;
    DDRB = 0x2f;

    setup_irrecv(1);

    for (;;) {
	if (irrecv_decode(&r)) {
	    process(&state, &r);

	    if (state.on) {
		led(state.led);
	    } else {
		led(0xff);
	    }

	    irrecv_resume();
	}
    }

    return 0;
}
