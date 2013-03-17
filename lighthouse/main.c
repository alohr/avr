#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <util/delay.h>

enum {
    DELAY = 800,

    WD_500_MILLIS = 0,
    WD_1_SECOND,
    WD_2_SECONDS,
    WD_8_SECONDS,
};

static void watchdog_enable(int seconds)
{
    cli();
    wdt_reset();

    MCUSR &= ~(_BV(WDRF)); /* clear WDRF in MCUSR */

    /* set up WDT interrupt */
    WDTCR = _BV(WDCE) | _BV(WDE);

    switch (seconds) {
    case WD_500_MILLIS:
	/* start watchdog timer with 500ms prescaler */
	WDTCR = _BV(WDIE) | _BV(WDP2) | _BV(WDP0);
	break;
    case WD_1_SECOND:
	/* start watchdog timer with 1s prescaler */
	WDTCR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1);
	break;
    case WD_2_SECONDS:
	/* start watchdog timer with 2s prescaler */
	WDTCR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	break;
    case WD_8_SECONDS:
	/* start watchdog timer with 8s prescaler */
	WDTCR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);
	break;
    }

    sei();
}

static void watchdog_disable(void)
{
    cli();
    wdt_reset();

    MCUSR &= ~(_BV(WDRF)); /* clear WDRF in MCUSR */
    WDTCR = _BV(WDCE) | _BV(WDE);
    WDTCR = 0;
    sei();
}

static void sleep(int mode)
{
    set_sleep_mode(mode);
    sleep_enable();
    sleep_mode();
    sleep_disable();
}

static void powerdown(void)
{
    sleep(SLEEP_MODE_PWR_DOWN);
}

void setup(void)
{
    MCUCR |= _BV(BODS); /* disable BOD in sleep mode */

    PRR |= _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSI);
    ADCSRA = 0;

    ACSR &= _BV(ACO); /* clear analog comparator output */
    DIDR0 |= _BV(AIN1D) | _BV(AIN0D); /* disable digital input */

    DDRB = _BV(PB2) | _BV(PB4); /* PB2, PB4 output */
    PORTB = ~(_BV(PB2) | _BV(PB4)); /* enable pull-ups */
}

int main(void)
{
    int blink = 0;

    setup();

    for (;;) {
	/* enable transistor */
	PORTB |= _BV(PB4);
	_delay_us(500);
	blink = ACSR & _BV(ACO); 
	PORTB &= ~(_BV(PB4));
	/* disable transistor */

	if (blink) {
	    PORTB |= _BV(PB2);
	    _delay_ms(DELAY);
	    PORTB &= ~(_BV(PB2));
	    watchdog_enable(WD_2_SECONDS);
	} else {
	    watchdog_enable(WD_8_SECONDS);
	}

	powerdown();
	watchdog_disable();
    }

    return 0;
}
