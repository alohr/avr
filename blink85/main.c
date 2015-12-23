#include <avr/io.h>
#include <util/delay.h>

static const int DELAY = 500;

int main(void)
{
    DDRB |= _BV(PB0);

    for (;;) {
	PORTB ^= _BV(PB0);
	_delay_ms(DELAY);
    }

    return 0;
}
