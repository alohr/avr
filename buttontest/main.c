#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>


void setup(void)
{
    DDRD |= _BV(PD5);
}

int main(void)
{
    setup();

    for (;;) {
	if (!(PIND & _BV(PD4)))
	    PORTD |= _BV(PD5);
	else
	    PORTD &= ~(_BV(PD5));
    }

    return 0;
}
