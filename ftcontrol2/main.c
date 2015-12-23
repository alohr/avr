/*
 * http://code.google.com/p/arduino-new-ping/
 */

#ifndef F_CPU
#error F_CPU not defined
#endif

#include <string.h>
#include <stdlib.h>
#include <limits.h>
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

    STEERING_TIMEOUT_MS = 105,
    MOTOR0_TIMEOUT_MS = 105,
    MOTOR1_MIN = 3,
    MOTOR1_MAX = 20,

    /* ping sensor */
    MAX_SENSOR_DELAY = 18000,
    MAX_SENSOR_DISTANCE_CM = 500,
    ROUNDTRIP_CM_US = 57    // micros it takes sound to travel round-trip 1cm (2cm total)
};

typedef struct {
    unsigned long t, t0;
} timer;

typedef struct {
    unsigned long data[5];
    int i;
    int n;
} counter;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define PRESCALE_ADJUST(x) ((x) << 1)

typedef struct {
    int toggle;

    // servo
    int turn;
    unsigned long timestamp;

    // motor0
    int run0;
    unsigned long timestamp0;

    // motor1
    int run1;
} state;


void update_counter(counter *c, int value)
{
    c->data[c->i++] = value;
    if (c->i == ARRAY_SIZE(c->data))
	c->i = 0;

    if (c->n < ARRAY_SIZE(c->data))
	c->n++;
}

unsigned long average_counter(const counter *c)
{
    unsigned long sum = 0;

    if (c->n == 0)
	return 0;

    for (int i = 0; i < c->n; i++)
	sum += c->data[i];

    return sum / c->n;
}

unsigned long ping_trigger(void)
{
    unsigned long t0 = 0;

    /* Set sensor pin to output. */
    DDRB |= _BV(PB4);
    PORTB &= ~(_BV(PB4)); /* Set sensor pin low. */
    _delay_us(4);

    PORTB |= _BV(PB4); /* Set sensor pin high. */
    _delay_us(10);
    PORTB &= ~(_BV(PB4)); /* Back to low. */

    /* Set sensor pin back as input. */
    DDRB &= ~(_BV(PB4));

    /* Wait for echo pin to clear. */
    t0 = micros();
    while (bit_is_set(PINB, PB4)) {
	if (micros() - t0 > MAX_SENSOR_DELAY)
	    return ULONG_MAX;
    }

    /* Wait for ping to start. */
    while (bit_is_clear(PINB, PB4)) {
	if (micros() - t0 > MAX_SENSOR_DELAY)
	    return ULONG_MAX;
    }

    /* Ping started */
    return micros();
}

unsigned long ping(int max_cm_distance)
{
    unsigned long t0 = 0;

    // Calculate the maximum distance in uS.
    if (max_cm_distance > MAX_SENSOR_DISTANCE_CM)
	max_cm_distance = MAX_SENSOR_DISTANCE_CM;

    unsigned long tmax =
	max_cm_distance * ROUNDTRIP_CM_US + (ROUNDTRIP_CM_US / 2);

    if ((t0 = ping_trigger()) <= 10)
	return ULONG_MAX;

    while (bit_is_set(PINB, PB4)) {
    	if (micros() - t0 > tmax)
    	    return ULONG_MAX;
    }

    // Calculate ping time, 5uS of overhead.
    return (micros() - t0 - 5);
}

void set_motor_pins(const state *s)
{
    if (s->run0 > 0) {
	// motor0 forward
	PORTD &= ~(_BV(PD4));
	PORTD |= _BV(PD3);
    } else if (s->run0 < 0) {
	// motor0 backward
	PORTD &= ~(_BV(PD3));
	PORTD |= _BV(PD4);
    } else {
	// motor0 off
	PORTD &= ~(_BV(PD3));
	PORTD &= ~(_BV(PD4));
    }

    if (s->run1) {
	// clear OC1B on compare match (set output to low)
	TCCR1A |= _BV(COM1B1);
	OCR1B = PRESCALE_ADJUST(abs(s->run1) * 1000);

	if (s->run1 > 0) {
	    // forward
	    PORTD &= ~_BV(PD6);
	    PORTD |= _BV(PD7);
	} else {
	    // backward
	    PORTD |= _BV(PD6);
	    PORTD &= ~_BV(PD7);
	}
    } else {
	// set OC1B to normal operation
	TCCR1A &= ~_BV(COM1B1);
	PORTB &= ~_BV(PB2);

	// motor1 off
	PORTD &= ~(_BV(PD6));
	PORTD &= ~(_BV(PD7));
    }
}

void set_servo_pins(state *s)
{
    int deflection = STEERING_MID_US;

    if (s->turn < 0) {
	if (millis() - s->timestamp > STEERING_TIMEOUT_MS) {
	    s->turn = 0;
	} else {
	    deflection = s->turn * STEERING_STEP_US + STEERING_MID_US;
	    if (deflection < STEERING_MIN_US)
		deflection = STEERING_MIN_US;
	    OCR1A = PRESCALE_ADJUST(deflection);
	}
    } else if (s->turn > 0) {
	if (millis() - s->timestamp > STEERING_TIMEOUT_MS) {
	    s->turn = 0;
	} else {
	    deflection = s->turn * STEERING_STEP_US + STEERING_MID_US;
	    if (deflection > STEERING_MAX_US)
		deflection = STEERING_MAX_US;
	    OCR1A = PRESCALE_ADJUST(deflection);
	}
    } else {
	OCR1A = PRESCALE_ADJUST(STEERING_MID_US);
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
		s->run1 = (s->run1 != 0) ? 0 : MOTOR1_MIN;
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
	case CHANNEL_UP:
	    s->run0 = 1;
	    s->timestamp0 = millis();
	    break;
	case CHANNEL_DOWN:
	    s->run0 = -1;
	    s->timestamp0 = millis();
	    break;
	case VOLUME_UP:
	    if (s->run1 == -MOTOR1_MIN)
		s->run1 = 0;
	    else if (s->run1 == 0)
		s->run1 = MOTOR1_MIN;
	    else if (s->run1 < MOTOR1_MAX)
		s->run1++;
	    break;
	case VOLUME_DOWN:
	    if (s->run1 == MOTOR1_MIN)
		s->run1 = 0;
	    else if (s->run1 == 0)
		s->run1 = -MOTOR1_MIN;
	    else if (s->run1 > -MOTOR1_MAX)
		s->run1--;
	    break;
	}
    }
}

void setup_pwm1(void)
{
    // fast PWM, mode 14
    TCCR1A = _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12);

    // clear OC1A on compare match (set output to low)
    TCCR1A |= _BV(COM1A1);

    // clear OC1B on compare match (set output to low)
    TCCR1A |= _BV(COM1B1);

    // set prescaler /8
    TCCR1B |= _BV(CS11);

    ICR1 = PRESCALE_ADJUST(STEERING_PERIOD_US);
    OCR1A = PRESCALE_ADJUST(STEERING_MID_US);
    OCR1B = 0;
}

int main(void)
{
    timer tping = { 0, 0 };
    decode_results r;
    state s;
    counter cping;

    memset(&r, 0, sizeof r);
    memset(&s, 0, sizeof s);

    memset(&cping, 0, sizeof cping);

    /*
     * PB0 ir receiver input
     * PB1 servo control out (OC1A)
     * PB2 motor2 enable (OC1B)
     * PB4 ping sensor
     *
     * PD3 motor1 control out A
     * PD4 motor1 control out B
     * PD6 motor2 control out A
     * PD7 motor2 control out B
     */

    DDRB = _BV(PB1) | _BV(PB2);
    PORTB = ~(_BV(PB1) | _BV(PB2) | _BV(PB4));  /* enable pull-ups, leave PB4 alone */

    DDRD = 0xd8;
    PORTD = 0x27; /* enable pull-ups */

    setup_timer0();
    setup_pwm1();
    setup_irrecv();

    for (;;) {
	if (irrecv_decode(&r)) {
	    irinterpret(&s, &r);
	    irrecv_resume();
	}

	// switch motor0 off after a short period of time
	if (s.run0 && millis() - s.timestamp0 > MOTOR0_TIMEOUT_MS)
	    s.run0 = 0;

	set_motor_pins(&s);
	set_servo_pins(&s);

	// ping every 50ms
	if ((tping.t = millis()) - tping.t0 > 50) {
	    tping.t0 = tping.t;

	    unsigned long us = ping(200);
	    if (us != ULONG_MAX) {
		// got a valid reading
 		unsigned long cm = us / ROUNDTRIP_CM_US;
		update_counter(&cping, cm);

		// motor1 running forward?
		if (s.run1 > MOTOR1_MIN) {
		    cm = average_counter(&cping);
		    if (cm < 40) {
			s.run1 = 0;
		    } else {
			int newrun1 = (cm - 40) / 4;
			if (newrun1 < s.run1)
			    if ((s.run1 = newrun1) < MOTOR1_MIN + 2)
				s.run1 = 0;
		    }
		    set_motor_pins(&s);
		}
 	    }
	}

    }

    return 0;
}
