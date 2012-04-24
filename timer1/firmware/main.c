#ifndef F_CPU
#error F_CPU not defined
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define CLKFUDGE 3      // fudge factor for clock interrupt overhead
#define CLK 65536      	// max value for clock (timer1)
#define PRESCALE 8      // timer 0 clock prescale
#define SYSCLOCK F_CPU	// CPU clock frequency

#define CLKSPERUSEC (SYSCLOCK/PRESCALE/1000000)   // timer clocks per microsecond

// clock timer reset value
#define USECPERTICK 50  // microseconds per clock interrupt tick

#define INIT_TIMER_COUNT1 (CLK - USECPERTICK * CLKSPERUSEC + CLKFUDGE)
#define RESET_TIMER1 TCNT1 = (uint16_t) (INIT_TIMER_COUNT1)


ISR(TIMER1_OVF_vect)
{
    PORTB ^= _BV(PB5);

    RESET_TIMER1;
}

void setup_timer1(void)
{
    TCCR1A = 0;
    TCCR1B = 0x02; /* prescale /8 */

    /* Timer1 overflow interrupt enable */
    TIMSK |= _BV(TOIE1);
    RESET_TIMER1;

    sei();

}

int main(void)
{
    DDRB = _BV(PB5);

    setup_timer1();

    for (;;) {
	_delay_ms(500);
    }

    return 0;
}
