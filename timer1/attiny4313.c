#ifndef F_CPU
#error F_CPU not defined
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define CLKFUDGE 3

ISR(TIMER1_OVF_vect)
{
    PORTD ^= _BV(PD5);
    TCNT1 = 65435U + CLKFUDGE;
}

void setup_timer1(void)
{
    TCCR1A = 0;
    TCCR1B = _BV(CS11);        // prescale /8
    TIMSK |= _BV(TOIE1);       // enable overflow interrupt
    TCNT1 = 65435U + CLKFUDGE; // initial start value
    sei();
}

int main(void)
{
    DDRD = _BV(PD5);

    setup_timer1();

    for (;;) {
        _delay_ms(500);
    }

    return 0;
}
