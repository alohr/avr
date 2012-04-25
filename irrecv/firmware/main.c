#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "irrecv.h"

#define ONKYO_RC_581S

#ifdef ONKYO_RC_581S
enum {
    CHANNEL_1    = 0x4BC0AB54,
    CHANNEL_2    = 0x4BC06B94,
    CHANNEL_3    = 0x4BC0EB14,
    CHANNEL_4    = 0x4BC01BE4,
    CHANNEL_5    = 0x4BC09B64,
    CHANNEL_6    = 0x4BC05BA4,
    CHANNEL_7    = 0x4BC0DB24,
    CHANNEL_8    = 0x4BC03BC4,
    CHANNEL_9    = 0x4BC0BB44,
    CHANNEL_DOWN = 0x4B20F807,
    CHANNEL_UP   = 0x4B207887,
    VOLUME_UP    = 0x4BC040BF,
    VOLUME_DOWN  = 0x4BC0C03F,
    MODE         = 0x4B2010EF,
    ON_OFF       = 0x4B20D32C,
    SLEEP        = 0x4BC0BA45,
};
#endif

#ifdef RM_130
enum {
    ON_OFF       = 0x0c,
    MUTE         = 0x0d,
    TV_AV        = 0x0b,
    VOLUME_UP    = 0x10,
    VOLUME_DOWN  = 0x11,
    CHANNEL_UP   = 0x20,
    CHANNEL_DOWN = 0x21
};
#endif

void process(decode_results *r)
{
    if (r->value == REPEAT)
	return;

#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__)

#ifdef ONKYO_RC_581S
    switch (r->value) {
    case CHANNEL_1:
	PORTD ^= _BV(PD2);
	break;
    case CHANNEL_2:
	PORTD ^= _BV(PD3);
	break;
    case CHANNEL_3:
	PORTD ^= _BV(PD4);
	break;
    }
#endif

#ifdef RM_130
    switch (r->value & 0xff) {
    case CHANNEL_UP:
	PORTD ^= _BV(PD2);
	break;
    case MUTE:
	PORTD ^= _BV(PD3);
	break;
    case CHANNEL_DOWN:
	PORTD ^= _BV(PD4);
	break;
    }
#endif


#else
    switch (r->value) {
    case CHANNEL_1:
	PORTB ^= _BV(PB5);
	break;
    case CHANNEL_2:
	PORTB ^= _BV(PB3);
	break;
    case CHANNEL_3:
	PORTB ^= _BV(PB4);
	break;
    }
#endif
}

//#define TEST_PB2

int main(void)
{
    decode_results r = { .value = 0 };

#ifdef __AVR_ATtiny4313__
    // PD2, PD3, PD4 = output
    DDRD |= _BV(PD2);
    DDRD |= _BV(PD3);
    DDRD |= _BV(PD4);
#else
    // PB5, PB3, PB4 = output
    /* DDRB |= _BV(PB5); */
    /* DDRB |= _BV(PB3); */
    /* DDRB |= _BV(PB4); */
#endif

    irrecv_setup();

    for (;;) {
	if (irrecv_decode(&r)) {
	    process(&r);
	    irrecv_resume();
	}
    }

    return 0;
}
