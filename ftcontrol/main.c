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
    FORWARD  = 0,
    BACKWARD = 1,

    LEFT  = 1,
    RIGHT = 2,

    STEERING_DIFF_MS = 350,
    STEERING_IDLE_MS = 1550,
    STEERING_MIN_MS = STEERING_IDLE_MS - STEERING_DIFF_MS,
    STEERING_MAX_MS = STEERING_IDLE_MS + STEERING_DIFF_MS,
    STEERING_TIMEOUT_MS = 100
};

typedef struct {
    uint8_t toggle;
    uint8_t run;
    uint8_t direction;
    uint8_t leftright;
    uint8_t leftright_state;
    uint8_t leftright_prev;
    unsigned long steering_time;
    unsigned long short_steering_time;
} state;

void set_pins(const state *s)
{
    if (s->run) {
	// main motor is running
	if (s->direction) {
	    PORTD &= ~(_BV(PD2)); 
	    PORTD |= _BV(PD3);
	} else {
	    PORTD &= ~(_BV(PD3)); 
	    PORTD |= _BV(PD2);
	}
    } else {
	// motor not running -> turn all off
	PORTD &= ~(_BV(PD3)); 
	PORTD &= ~(_BV(PD2)); 

	PORTB &= ~(_BV(PB3)); 
	PORTB &= ~(_BV(PB2)); 
    }

    if (s->direction) {
	PORTA &= ~(_BV(PA0));
	PORTA |= _BV(PA1);
    } else {
	PORTA &= ~(_BV(PA1)); 
	PORTA |= _BV(PA0);
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

void process(state *s, const decode_results *r)
{
    if (r->decode_type == RC5) {
	uint8_t toggle = (r->value & 0x800) != 0;

	if (toggle != s->toggle) {
	    s->toggle = toggle;

	    switch (r->value & 0xff) {
	    case ON_OFF:
		s->run = !s->run;
		if (!s->run)
		    s->leftright = 0;
		break;
	    case CHANNEL_UP:
		s->direction = FORWARD;
		break;
	    case CHANNEL_DOWN:
		s->direction = BACKWARD;
		break;
	    case MUTE:
		// left
		s->leftright = LEFT;
		s->leftright_state = 1;
		s->steering_time = millis();
		s->short_steering_time = 0;
		break;
	    case TV_AV:
		// right
		s->leftright = RIGHT;
		s->leftright_state = 1;
		s->steering_time = millis();
		s->short_steering_time = 0;
		break;
	    }
	} else {
	    switch (r->value & 0xff) {
	    case VOLUME_DOWN:
		// short left
		s->leftright = LEFT;
		s->leftright_state = 0;
		s->steering_time = 0;
		s->short_steering_time = millis();
		break;
	    case VOLUME_UP:
		// short right
		s->leftright = RIGHT;
		s->leftright_state = 0;
		s->steering_time = 0;
		s->short_steering_time = millis();
		break;
	    }
	}
	set_pins(s);
    }

}

void setup_pwm(void)
{
    // Fast PWM, mode 14
    TCCR1A = 0;
    TCCR1B = 0;
//    TCCR1A = _BV(WGM11);
//    TCCR1B = _BV(WGM13) | _BV(WGM12);


    // Clear OC1B on compare match (set output to low)
    // TCCR1A |= _BV(COM1B1);
    TCCR1A |= _BV(COM1A1);

    // Prescaling is set in setup_irrecv()
    ICR1 = 20000; // 20 ms
    OCR1B = STEERING_IDLE_MS;
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
     *  PORTA  0 0 0 0  0 0 1 1 = 0x03
     *  PORTB  0 0 1 1  1 1 0 0 = 0x3c (~ 0xc3)
     *  PORTD  0 0 0 0  1 1 0 0 = 0x0c (~ 0xf3)
     *
     */

    DDRB = 0x3c;
    PORTB |= 0xc3; /* enable pullup */

    DDRD = 0x0c;
    PORTD |= 0xf3; /* enable pullup */

    DDRA |= 0x03;

    setup_timer0();
    setup_pwm();
    setup_irrecv();

    set_pins(&s);

    for (;;) {
	if (irrecv_decode(&r)) {
	    process(&s, &r);
	    irrecv_resume();
	}
    }

    return 0;
}
