#ifndef F_CPU
#error F_CPU not defined
#endif

#include <string.h>
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

    STEERING_DIFF_US = 350,
    STEERING_STEP_US = 50,
    STEERING_MID_US = 1530,
    STEERING_MIN_US = STEERING_MID_US - STEERING_DIFF_US,
    STEERING_MAX_US = STEERING_MID_US + STEERING_DIFF_US,
    STEERING_PERIOD_US = 20000,

    STEERING_TIMEOUT_MS = 105
};

#define PRESCALE_ADJUST(x) ((x) << 1)

typedef struct {
    int toggle;

    // servo
    int turn;
    unsigned long timestamp;

    // motor1
    int run;
    int direction;

} state;

void set_motor_pins(const state *s)
{
    if (s->run) {
	// motor1 is running
	if (s->direction) {
	    PORTD &= ~(_BV(PD4)); 
	    PORTD |= _BV(PD3);
	} else {
	    PORTD &= ~(_BV(PD3)); 
	    PORTD |= _BV(PD4);
	}
    } else {
	// motor not running -> turn all off
	PORTD &= ~(_BV(PD3)); 
	PORTD &= ~(_BV(PD4)); 
    }
}

void irinterpret(state *s, const decode_results *r)
{
    if (r->decode_type == RC5) {
	int toggle = (r->value & 0x800) != 0;

	if (toggle != s->toggle) {
	    s->toggle = toggle;

	    switch (r->value & 0xff) {
	    case ON_OFF:
		s->run = !s->run;
		break;
	    case CHANNEL_UP:
		s->direction = FORWARD;
		break;
	    case CHANNEL_DOWN:
		s->direction = BACKWARD;
		break;
	    }
	}

	switch (r->value & 0xff) {
	case TV_AV:
	    // turn servo left (min)
	    s->turn = -STEERING_DIFF_US / STEERING_STEP_US;
	    s->timestamp = millis();
	    break;
	case MUTE:
	    // turn servo right (max)
	    s->turn = STEERING_DIFF_US / STEERING_STEP_US;
	    s->timestamp = millis();
	    break;
	}
    }
}

void setup_pwm(void)
{
    // fast PWM, mode 14
   TCCR1A = _BV(WGM11);
   TCCR1B = _BV(WGM13) | _BV(WGM12);

   // clear OC1B on compare match (set output to low)
   TCCR1A |= _BV(COM1A1);

   // set prescaler /8
   TCCR1B |= _BV(CS11);

   ICR1 = PRESCALE_ADJUST(STEERING_PERIOD_US);
   OCR1A = PRESCALE_ADJUST(STEERING_MID_US);
}

int main(void)
{
    decode_results r;
    state s;

    memset(&r, 0, sizeof r);
    memset(&s, 0, sizeof s);

    /*
     * PB0 ir receiver input
     * PB1 servo control out
     * PB4 ir indicator led
     * PB5 indicator led
     * 
     * PD3 motor1 control out A
     * PD4 motor1 control out B
     */

    DDRB = _BV(PB1) | _BV(PB4) | _BV(PB5);
    PORTB = ~(_BV(PB1) | _BV(PB4) | _BV(PB5)); /* enable pull-ups */
    
    DDRD = _BV(PD3) | _BV(PD4);
    PORTD = ~(_BV(PD3) | _BV(PD4)); /* enable pull-ups */

    setup_timer0();
    setup_pwm();
    setup_irrecv();

    int deflection = STEERING_MID_US;

    for (;;) {
	if (irrecv_decode(&r)) {
	    irinterpret(&s, &r);
	    irrecv_resume();
	}

	set_motor_pins(&s);

	if (s.turn < 0) {
	    if (millis() - s.timestamp > STEERING_TIMEOUT_MS) {
		s.turn = 0;
	    } else {
		deflection = s.turn * STEERING_STEP_US + STEERING_MID_US;
		if (deflection < STEERING_MIN_US)
		    deflection = STEERING_MIN_US;
		OCR1A = PRESCALE_ADJUST(deflection);
	    }
	} else if (s.turn > 0) {
	    if (millis() - s.timestamp > STEERING_TIMEOUT_MS) {
		s.turn = 0;
	    } else {
		deflection = s.turn * STEERING_STEP_US + STEERING_MID_US;
		if (deflection > STEERING_MAX_US)
		    deflection = STEERING_MAX_US;
		OCR1A = PRESCALE_ADJUST(deflection);
	    }
	} else {
	    OCR1A = PRESCALE_ADJUST(STEERING_MID_US);
	}
    }

    return 0;
}
