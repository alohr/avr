#ifndef F_CPU
#error F_CPU not defined
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "timer0.h"

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( ((a) * 1000L) / (F_CPU / 1000L) )
#define microsecondsToClockCycles(a) ( ((a) * (F_CPU / 1000L)) / 1000L )

#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;


ISR(TIMER0_OVF_vect)
{
    // copy these to local variables so they can be stored in registers
    // (volatile variables must be read from memory on every access)
    unsigned long m = timer0_millis;
    unsigned char f = timer0_fract;

    m += MILLIS_INC;
    f += FRACT_INC;
    if (f >= FRACT_MAX) {
	f -= FRACT_MAX;
	m += 1;
    }

    timer0_fract = f;
    timer0_millis = m;
    timer0_overflow_count++;
}

unsigned long millis(void)
{
    unsigned long m;
    uint8_t oldSREG = SREG;

    // disable interrupts while we read timer0_millis or we might get an
    // inconsistent value (e.g. in the middle of a write to timer0_millis)
    cli();
    m = timer0_millis;
    SREG = oldSREG;

    return m;
}

unsigned long micros(void)
{
    unsigned long m;
    uint8_t oldSREG = SREG, t;
	
    cli();
    m = timer0_overflow_count;
    t = TCNT0;

#ifdef TIFR0
    if ((TIFR0 & _BV(TOV0)) && (t < 255))
	m++;
#else
    if ((TIFR & _BV(TOV0)) && (t < 255))
	m++;
#endif

    SREG = oldSREG;
	
    return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
}

void setup_timer0(void)
{
    TCCR0A = 0;
    TCCR0B = 0x03; /* prescale /64 */

    /* Timer0 overflow interrupt enable */
    TIMSK |= _BV(TOIE0);
    sei();
}
