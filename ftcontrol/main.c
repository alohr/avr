#ifndef F_CPU
#error F_CPU not defined
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "irrecv.h"
#include "timer0.h"
#include "board.h"

enum {
    ON_OFF       = 0x0c,
    MUTE         = 0x0d,
    TV_AV        = 0x0b,
    VOLUME_UP    = 0x10,
    VOLUME_DOWN  = 0x11,
    CHANNEL_UP   = 0x20,
    CHANNEL_DOWN = 0x21,
};

enum {
    LEFT  = 1,
    RIGHT = 2,
};

typedef struct {
    uint8_t toggle;
    uint8_t run;
    uint8_t direction;
    uint8_t leftright;
    unsigned long steering_time;
    unsigned long toggle_time;
} state;

void set_pins(const state *s)
{
    if (s->run) {
	if (s->direction) {
	    PORTD &= ~(_BV(PD2)); 
	    PORTD |= _BV(PD3);
	} else {
	    PORTD &= ~(_BV(PD3)); 
	    PORTD |= _BV(PD2);
	}
    } else {
	PORTD &= ~(_BV(PD3)); 
	PORTD &= ~(_BV(PD2)); 

	PORTB &= ~(_BV(PB3)); 
	PORTB &= ~(_BV(PB2)); 

    }

    if (s->leftright == LEFT) {
	PORTB &= ~(_BV(PB3)); 
	PORTB |= _BV(PB2);
    } else if (s->leftright == RIGHT) {
	PORTB &= ~(_BV(PB2)); 
	PORTB |= _BV(PB3);
    } else {
	PORTB &= ~(_BV(PB3)); 
	PORTB &= ~(_BV(PB2)); 
    }
}

void process(state *s, decode_results *r)
{
    unsigned long t;
    
    if (r->decode_type == RC5) {
	uint8_t toggle = (r->value & 0x800) != 0;

	if (toggle != s->toggle) {
	    s->toggle = toggle;

	    switch (r->value & 0xff) {
	    case ON_OFF:
		if ((t = millis()) - s->toggle_time > 400) {
		    s->toggle_time = t;
		    s->run = !s->run;
		    if (!s->run)
			s->leftright = 0;
		}
		break;
	    case CHANNEL_UP:
		s->direction = 0;
		break;
	    case CHANNEL_DOWN:
		s->direction = 1;
		break;
	    }
	}

	switch (r->value & 0xff) {
	case MUTE:
	    s->leftright = LEFT;
	    s->steering_time = millis();
	    break;
	case TV_AV:
	    s->leftright = RIGHT;
	    s->steering_time = millis();
	    break;
	}

	set_pins(s);
    }
}

int main(void)
{
    decode_results r = {
	.value = 0
    };

    state s = {
	.toggle = 0xff,
	.run = 0,
	.direction = 0
    };

    /*
     *         7 6 5 4  3 2 1 0
     *  PORTB  0 0 1 0  1 1 0 0 = 0x2c (~ 0xd3)
     *  PORTD  0 0 0 0  1 1 0 0 = 0x0c (~ 0xf3)
     *
     */

    DDRB = 0x2c;
    PORTB |= 0xd3; /* enable pullup */

    DDRD = 0x0c;
    PORTD |= 0xf3; /* enable pullup */

    setup_timer0();
    setup_irrecv();

    for (;;) {
	if (irrecv_decode(&r)) {
	    process(&s, &r);
	    irrecv_resume();
	} else if (millis() - s.steering_time > 200) {
	    s.leftright = 0;
	    set_pins(&s);
	}
    }

    return 0;
}
