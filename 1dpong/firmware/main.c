#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

enum {
    DELAY_MS_DEBOUNCE = 50,
    DELAY_MS_MOVE = 300,

    LED_MIN = 0,
    LED_MAX = 11,
    NUM_LEDS = 12,

    PLAYER_0 = 0,
    PLAYER_1 = 1,
};

uint8_t direction[] = {
    0b0011, // LED1
    0b0011, // LED2
    0b0101, // LED3
    0b0101, // LED4
    0b0110, // LED5
    0b0110, // LED6

    0b1001, // LED7
    0b1001, // LED8
    0b1010, // LED9
    0b1100, // LED11
    0b1010, // LED10
    0b1100, // LED12
};

uint8_t port[] = {
    0b0001, // LED1
    0b0010, // LED2
    0b0001, // LED3
    0b0100, // LED4
    0b0010, // LED5
    0b0100, // LED6

    0b0001, // LED7
    0b1000, // LED8
    0b0010, // LED9
    0b0100, // LED11
    0b1000, // LED10
    0b1000, // LED12
};

volatile int button0_pressed = 0;
volatile int button1_pressed = 0;

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

void loser_sound(void)
{
    buzz_sound(255, 1000);
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

/* void beep(void) */
/* { */
/*     TCCR0A |= _BV(COM0B1); */
/*     _delay_ms(50); */
/*     TCCR0A &= ~_BV(COM0B1); */
/* } */

ISR(INT0_vect)
{
    if (bit_is_clear(PIND, PD2)) {
	_delay_ms(DELAY_MS_DEBOUNCE);
	if (bit_is_clear(PIND, PD2)) {
	    ++button0_pressed;
	    toner(2, 5);
	}
    }
}

ISR(INT1_vect)
{
    if (bit_is_clear(PIND, PD3)) {
    	_delay_ms(DELAY_MS_DEBOUNCE);
    	if (bit_is_clear(PIND, PD3)) {
	    ++button1_pressed;
	    toner(3, 5);
    	}
    }
}

void flash(int n)
{
    while (n-- > 0) {
	for (int i = 5, j = 6; i > 0; --i, ++j) {
	    DDRB = direction[i];
	    PORTB = port[i];
	    _delay_ms(20);

	    DDRB = direction[j];
	    PORTB = port[j];
	    _delay_ms(20);
	}
    }
}

void reset_buttons()
{
    cli();
    button0_pressed = 0;
    button1_pressed = 0;
    sei();
}

int check_button(int i)
{
    int pressed = 0;

    cli();
    switch (i) {
    case 0:
	pressed = (button0_pressed != 0);
	button0_pressed = 0;
	break;
    case 1:
	pressed = (button1_pressed != 0);
	button1_pressed = 0;
	break;
    }
    sei();

    return pressed;
}

void led(int i)
{
    DDRB = direction[i];
    PORTB = port[i];
}

void setup(void)
{
    DDRB = 0;
    PORTB = 0;

    DDRD = _BV(PD5); /* PD5 output (piezo) */
    PORTD |= _BV(PD2); /* enable pull-up on PD2 - INT0 */
    PORTD |= _BV(PD3); /* enable pull-up on PD3 - INT1 */

    /* INT1 on falling edge */
    MCUCR |= _BV(ISC11);
    MCUCR &= ~_BV(ISC10);

    /* INT0 on falling edge */
    MCUCR |= _BV(ISC01);
    MCUCR &= ~_BV(ISC00);

    GIMSK |= (_BV(INT1) | _BV(INT0)); /* enable INT1, INT0 */
   
    /* PWM */
    TCCR0A |= (_BV(WGM01) | _BV(WGM00));
    TCCR0B |= _BV(CS01);
    TCNT0 = 0;
    OCR0B = 50;
      
    sei();
}

int mainloop(int start_player)
{
    int i = 0;
    int inc = 1;
    int good = 1;
    int delay = 10;

    reset_buttons();

    if (start_player == 0) {
	i = LED_MIN;
	inc = 1;
    } else {
	i = LED_MAX;
	inc = -1;
    }

    for (;;) {
	led(i);

	switch (delay) {
	case 10: _delay_ms(DELAY_MS_MOVE); break;
	case  9: _delay_ms(DELAY_MS_MOVE-20); break;
	case  8: _delay_ms(DELAY_MS_MOVE-40); break;
	case  7: _delay_ms(DELAY_MS_MOVE-60); break;
	case  6: _delay_ms(DELAY_MS_MOVE-80); break;
	case  5: _delay_ms(DELAY_MS_MOVE-100); break;
	case  4: _delay_ms(DELAY_MS_MOVE-120); break;
	case  3: _delay_ms(DELAY_MS_MOVE-140); break;
	case  2: _delay_ms(DELAY_MS_MOVE-160); break;
	case  1: _delay_ms(DELAY_MS_MOVE-180); break;
	default: _delay_ms(DELAY_MS_MOVE-200);
	}

	switch (i) {
	case LED_MIN:
	    if ((good = (inc == 1 || check_button(0)))) {
		inc = 1;
		--delay;
	    }
	    break;
	case LED_MAX:
	    if ((good = (inc == -1 || check_button(1)))) {
		inc = -1;
		--delay;
	    }
	    break;
	default:
	    good = !(check_button(0) || check_button(1));
	    break;
	}
	
	if (!good) {
	    /* who won? */
	    loser_sound();
	    return inc == 1 ? PLAYER_0 : PLAYER_1;
	}

	i += inc;
    }
}

int main(void)
{
    int start_player = 0;

    setup();

    DDRD |= _BV(BUZZER1);
    DDRD |= _BV(BUZZER2);

    winner_sound();

    for (;;) {
	flash(3);
	start_player = mainloop(start_player);
    }

    return 0;
}
