#include <avr/io.h>
#include <util/delay.h>

static const int DELAY = 500;

void ledon(int i)
{
    PORTB |= _BV(i);
    _delay_ms(DELAY);
    PORTB &= ~_BV(i);
}

int main(void)
{
    DDRB |= 0xff;

    for (;;) {
        for (int i = 0; i < 8; i++) {
            ledon(i);
        }
    }

    return 0;
}
