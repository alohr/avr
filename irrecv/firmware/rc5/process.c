#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>

#include <irrecv.h>
#include <timer0.h>
#include <state.h>

enum {
    ON_OFF       = 0x0c,
    MUTE         = 0x0d,
    TV_AV        = 0x0b,
    VOLUME_UP    = 0x10,
    VOLUME_DOWN  = 0x11,
    CHANNEL_UP   = 0x20,
    CHANNEL_DOWN = 0x21,

    MIN_DELAY    = 10,
    MAX_DELAY    = 500,
    DELAY_STEP   = 10
};

void process(ledstate *state, const decode_results *r)
{
    uint8_t toggle = (r->value & 0x800) != 0;
    unsigned long t;

    if (toggle != state->toggle) {
	state->toggle = toggle;

	switch (r->value & 0xff) {
	case CHANNEL_UP:
	    if (++state->led == 9)
		state->led = 0;
	    break;
	case CHANNEL_DOWN:
	    if (--state->led < 0)
		state->led = 8;
	    break;
	case VOLUME_UP:
	    if (state->delay_time > MIN_DELAY)
		state->delay_time -= DELAY_STEP;
	    break;
	case VOLUME_DOWN:
	    if (state->delay_time < MAX_DELAY)
		state->delay_time += DELAY_STEP;
	    break;
	case TV_AV:
	    state->flags.run = !state->flags.run;
	    break;
	case ON_OFF:
	    if ((t = millis()) - state->toggle_time > 500) {
		state->toggle_time = t;
		state->flags.on = !state->flags.on;
	    }
	    break;
	}
    }
}
