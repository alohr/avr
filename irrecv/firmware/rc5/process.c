#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>

#include <irrecv.h>
#include <state.h>

enum {
    ON_OFF       = 0x0c,
    MUTE         = 0x0d,
    TV_AV        = 0x0b,
    VOLUME_UP    = 0x10,
    VOLUME_DOWN  = 0x11,
    CHANNEL_UP   = 0x20,
    CHANNEL_DOWN = 0x21
};

void process(ledstate *state, const decode_results *r)
{
    uint8_t toggle = (r->value & 0x800) != 0;

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
	case ON_OFF:
	    state->on = !state->on;
	    break;
	}
    }
}
