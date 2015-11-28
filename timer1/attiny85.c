#ifndef F_CPU
#error F_CPU not defined
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define CLKFUDGE 3

ISR(TIMER1_OVF_vect)
{
    PORTB ^= _BV(PB0);
    TCNT1 = 155U + CLKFUDGE;
}

void setup_timer1(void)
{
    TCCR1 = _BV(CS12);   // prescale by using peripheral clock /8
    TIMSK |= _BV(TOIE1); // enable overflow interrupt
    TCNT1 = 155U + CLKFUDGE;
    sei();
}

int main(void)
{
    DDRB = _BV(PB0);

    setup_timer1();

    for (;;) {
        _delay_ms(500);
    }

    return 0;
}
