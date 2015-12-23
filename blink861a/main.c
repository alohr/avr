#include <avr/io.h>
#include <util/delay.h>

static const int DELAY = 500;

int main(void)
{
    DDRA |= _BV(PA6);

    for (;;) {
	PORTA ^= _BV(PA6);
	_delay_ms(DELAY);
    }

    return 0;
}
