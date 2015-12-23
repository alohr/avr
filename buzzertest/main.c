#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

/* Buzzer pin definitions */
#define BUZZER1		4
#define BUZZER1_PORT	PORTD
#define BUZZER2		5
#define BUZZER2_PORT	PORTD

static void delay_us(uint16_t x)
{
    while (x-- > 0) {
	_delay_us(1);
    }
}

static void winner_sound(void)
{
    uint8_t x, y;

    /* Toggle the buzzer at various speeds */
    for (x = 250; x > 70; x--) {
	for (y = 0; y < 3; y++) {
	    BUZZER2_PORT |= _BV(BUZZER2);
	    BUZZER1_PORT &= ~(_BV(BUZZER1));

	    delay_us(x);

	    BUZZER2_PORT &= ~(_BV(BUZZER2));
	    BUZZER1_PORT |= _BV(BUZZER1);
	    
	    delay_us(x);
	}
    }
}

static void buzz_sound(uint16_t buzz_length_ms, uint16_t buzz_delay_us)
{
    uint32_t buzz_length_us;

    buzz_length_us = buzz_length_ms * (uint32_t)1000;

    while (buzz_length_us > buzz_delay_us*2) {
	buzz_length_us -= buzz_delay_us*2;

	/* toggle the buzzer at various speeds */
	BUZZER1_PORT &= ~(_BV(BUZZER1));
	BUZZER2_PORT |= _BV(BUZZER2);

	delay_us(buzz_delay_us);

	BUZZER1_PORT |= _BV(BUZZER1);
	BUZZER2_PORT &= ~(_BV(BUZZER2));
	delay_us(buzz_delay_us);
    }
}

void play_loser(void)
{
    buzz_sound(255, 1500);
    buzz_sound(255, 1500);
    buzz_sound(255, 1500);
    buzz_sound(255, 1500);
}

static void toner(uint8_t which, uint16_t buzz_length_ms)
{
    switch (which) {
    case 0:
	buzz_sound(buzz_length_ms, 1136); 
	break;
    
    case 1:
	buzz_sound(buzz_length_ms, 568); 
	break;

    case 2:
	buzz_sound(buzz_length_ms, 851); 
	break;

    case 3:
	buzz_sound(buzz_length_ms, 638); 
	break;
    }
}

void setup(void)
{
    DDRD |= _BV(BUZZER1);
    DDRD |= _BV(BUZZER2);
    
    DDRB = _BV(PB0);
    PORTB = ~(_BV(PB0)); /* enable pull-ups */
}

int main(void)
{
    setup();

    for (;;) {
 	/* winner_sound(); */
	/* winner_sound(); */
	/* winner_sound(); */
	/* winner_sound(); */
	/* _delay_ms(1000); */

	/* toner(0, 50); */
	/* _delay_ms(1000); */

	toner(1, 50);
	_delay_ms(1000);

 	toner(2, 50);
	_delay_ms(1000);

	/* toner(3, 50); */
	/* _delay_ms(1000); */

	/* play_loser(); */
	/* _delay_ms(1000); */
    }

    return 0;
}
