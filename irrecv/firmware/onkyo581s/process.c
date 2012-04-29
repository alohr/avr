#include <inttypes.h>

#include <irrecv.h>
#include <state.h>

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

void process(ledstate *state, const decode_results *r)
{
    if (r->value == REPEAT)
	return;

    switch (r->value) {
    case CHANNEL_1:
	state->on = 1;
	state->led = 0;
	break;
    case CHANNEL_2:
	state->on = 1;
	state->led = 1;
	break;
    case CHANNEL_3:
	state->on = 1;
	state->led = 2;
	break;
    case CHANNEL_4:
	state->on = 1;
	state->led = 3;
	break;
    case CHANNEL_5:
	state->on = 1;
	state->led = 4;
	break;
    case CHANNEL_6:
	state->on = 1;
	state->led = 5;
	break;
    case CHANNEL_7:
	state->on = 1;
	state->led = 6;
	break;
    case CHANNEL_8:
	state->on = 1;
	state->led = 7;
	break;
    case CHANNEL_9:
	state->on = 1;
	state->led = 8;
	break;
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
