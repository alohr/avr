#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <util/delay.h>

enum {
    DELAY = 800
};

void watchdog_enable(void)
{
    cli();
    wdt_reset();

    MCUSR &= ~(_BV(WDRF)); /* clear WDRF in MCUSR */

    /* set up WDT interrupt */
    WDTCR = _BV(WDCE) | _BV(WDE);

    /* start watchdog timer with 2s prescaller */
    WDTCR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
    sei();
}

void watchdog_disable(void)
{
    cli();
    wdt_reset();

    MCUSR &= ~(_BV(WDRF)); /* clear WDRF in MCUSR */
    WDTCR = _BV(WDCE) | _BV(WDE);
    WDTCR = 0;
    sei();
}

void powerdown(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    sleep_mode();
    sleep_disable();
}

int main(void)
{
    DDRB = _BV(PB0); /* PB0 output */
    PORTB &= ~(_BV(PB0)); /* enable pull-ups */

    for (;;) {
	PORTB |= _BV(PB0);
	_delay_ms(DELAY);
	PORTB &= ~(_BV(PB0));

	watchdog_enable();
	powerdown();
	watchdog_disable();
    }

    return 0;
}
