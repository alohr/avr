#include <avr/io.h>
#include <util/delay.h>

static const int DELAY_SHORT = 100;
static const int DELAY = 500;

int main(void)
{
    int n = 10;

    DDRB |= _BV(PB0);

    while (n-- > 0) {
	PORTB ^= _BV(PB0);
	_delay_ms(DELAY_SHORT);
    }

    for (;;) {
	PORTB ^= _BV(PB0);
	_delay_ms(DELAY);
    }

    return 0;
}
