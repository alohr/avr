#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "irrecv.h"
#include "timer0.h"

#define R0 PC5
#define G0 PC4
#define B0 PC3

#define R1 PB1
#define G1 PB2
#define B1 PB3

enum {
    DELAY = 500,
    FLASH = 60
};

enum {
    ON_OFF    = 0x22D64AB5,
    EPG       = 0x22D632CD,
    ARCHIV    = 0x22D6A857,
    MARKER    = 0x22D6D02F,
    RED       = 0x22D67887,
    GREEN     = 0x22D652AD, 
    YELLOW    = 0x22D658A7, 
    BLUE      = 0x22D65AA5,

    CHANNEL_1 = 0x22D608F7,
    CHANNEL_2 = 0x22D610EF,
    CHANNEL_3 = 0x22D618E7,
};

typedef struct {
    long t0;
    int r0, g0, b0;
    int r1, g1, b1;
} state;

decode_results r;
state s;

void setup(void)
{
    DDRC |= (_BV(PC3) | _BV(PC4) | _BV(PC5));
    PORTC &= ~(_BV(PC3) | _BV(PC4) | _BV(PC5)); // enable pull-ups

    DDRB |= (_BV(PB1) | _BV(PB2) | _BV(PB3));
    PORTB &= ~(_BV(PB1) | _BV(PB2) | _BV(PB3)); // enable pull-ups
}

void process(state *s, const decode_results *r)
{
    long t = 0;

    if (r->value == REPEAT)
	return;

    if ((t = millis()) - s->t0 > 150) {
	s->t0 = t;

	switch (r->value) {
	case ON_OFF:
	    if (s->r0 || s->g0 || s->b0) {
		s->r0 = s->g0 = s->b0 = 0;
		s->r1 = s->g1 = s->b1 = 0;
	    } else {
		s->r0 = s->g0 = s->b0 = 1;
		s->r1 = s->g1 = s->b1 = 1;
	    }
	    break;
	case RED:
	    s->r0 = !s->r0;
	    s->r1 = !s->r1;
	    break;
	case GREEN:
	    s->g0 = !s->g0;
	    s->g1 = !s->g1;
	    break;
	case BLUE:
	    s->b0 = !s->b0;
	    s->b1 = !s->b1;
	    break;
	}
    }    
    if (s->r0) PORTC |= _BV(R0); else PORTC &= ~_BV(R0);
    if (s->r1) PORTB |= _BV(R1); else PORTB &= ~_BV(R1);

    if (s->g0) PORTC |= _BV(G0); else PORTC &= ~_BV(G0);
    if (s->g1) PORTB |= _BV(G1); else PORTB &= ~_BV(G1);

    if (s->b0) PORTC |= _BV(B0); else PORTC &= ~_BV(B0);
    if (s->b1) PORTB |= _BV(B1); else PORTB &= ~_BV(B1);
}

int main(void)
{
    long t0 = 0, t = 0;

    setup_timer0();
    setup_irrecv();
    setup();

    for (;;) {
	if (irrecv_decode(&r)) {
	    process(&s, &r);
	    irrecv_resume();
	}

	if ((t = millis()) - t0 > DELAY) {
	    t0 = t;
	}
    }

    return 0;
}
