#ifndef F_CPU
#error F_CPU not defined
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "timer0.h"

enum {
    PIN_ANODE_ONES = PC0,
    PIN_ANODE_TENS = PC1,
    PIN_CATHODE_HUNDREDS_A = PC2,
    PIN_CATHODE_HUNDREDS_B = PC3,
    PIN_LED_RED = PD6,
    PIN_LED_BLUE = PD7,
    PIN_SW1 = PD2,
    PIN_SW2 = PD3,

    SEGMENT_A = 0,
    SEGMENT_B = 1,
    SEGMENT_C = 2,
    SEGMENT_D = 3,
    SEGMENT_E = 4,
    SEGMENT_F = 5,
    SEGMENT_G = 6,

    TIMER2_RESET_TO_400_MICROS = 255 - 100,
};

static int digit[] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
};

typedef struct segment {
    uint8_t port_b;
    uint8_t port_c;
    int d;
} segment_t;

static segment_t segment[] = {
    { 0b0000001, 0b1101, 0 }, // a ones
    { 0b0000010, 0b1101, 0 }, // b ones
    { 0b0000100, 0b1101, 0 }, // c ones
    { 0b0001000, 0b1101, 0 }, // d ones
    { 0b0010000, 0b1101, 0 }, // e ones
    { 0b0100000, 0b1101, 0 }, // f ones
    { 0b1000000, 0b1101, 0 }, // g ones

    { 0b0000001, 0b1110, 1 }, // a tens
    { 0b0000010, 0b1110, 1 }, // b tens
    { 0b0000100, 0b1110, 1 }, // c tens
    { 0b0001000, 0b1110, 1 }, // d tens
    { 0b0010000, 0b1110, 1 }, // e tens
    { 0b0100000, 0b1110, 1 }, // f tens
    { 0b1000000, 0b1110, 1 }, // g tens

    { 0b0000000, 0b1010, 2 }, // b hundreds
    { 0b0000000, 0b0110, 2 }, // c hundreds
};

typedef struct {
    uint8_t digits[3];
    uint8_t segment;
    int value;
    int on;
} display_t;

static volatile display_t display;

typedef struct {
    int raw, prevraw;
    int v;
    long t;
} analogvalue_t;


void setup(void)
{
    DDRB = 0xff;
    PORTB = 0xff;

    DDRC = _BV(PIN_ANODE_ONES) |
           _BV(PIN_ANODE_TENS) |
           _BV(PIN_CATHODE_HUNDREDS_A) |
           _BV(PIN_CATHODE_HUNDREDS_B);

    PORTC = _BV(PIN_CATHODE_HUNDREDS_A) |
            _BV(PIN_CATHODE_HUNDREDS_B);

    DDRD = _BV(PIN_LED_RED) |
           _BV(PIN_LED_BLUE);

    PORTD = _BV(PIN_SW1) | _BV(PIN_SW2); // enable pull-ups

    // Set ADC prescaler /128, 16 Mhz / 128 = 125 KHz which is inside
    // the desired 50-200 KHz range.

    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}

void setup_timer2(void)
{
#if F_CPU == 8000000
    // prescale /32
    TCCR2A = 0;
    TCCR2B = _BV(CS21) | _BV(CS20);
#elif F_CPU == 16000000
    // prescale /64
    TCCR2A = 0;
    TCCR2B = _BV(CS22);
#else
#error F_CPU not recognized
#endif

    // overflow interrupt enable
    TIMSK2 |= _BV(TOIE2);
    TCNT2 = TIMER2_RESET_TO_400_MICROS;

    sei();
}

void analog_init(analogvalue_t *value)
{
    value->raw = 0;
    value->prevraw = 0;
    value->v = 0;
    value->t = millis();
}

int analog_read(int chan, analogvalue_t *value)
{
    uint8_t low, high;

    // AVCC with external capacitor at AREF pin, select channel
    ADMUX = (_BV(REFS0) | (chan & 0x0f));

    // start single conversion
    ADCSRA |= _BV(ADSC);

    // wait for conversion to complete
    loop_until_bit_is_clear(ADCSRA, ADSC);

    low  = ADCL;
    high = ADCH;

    value->raw = (high << 8) | low;
    value->t = millis();

    return value->raw;
}

int getdelay(analogvalue_t *value)
{
    analogvalue_t newvalue;
    enum { N_READINGS = 3 };

    int raw = 0;
    for (int i = 0; i < N_READINGS; i++) {
        analog_read(4, &newvalue);
        raw += newvalue.raw;
    }

    newvalue.raw = raw / N_READINGS;
    if (newvalue.raw > 995)
        newvalue.raw = 995;

    if ((newvalue.raw > value->prevraw && newvalue.raw - value->prevraw > 3) ||
        (newvalue.raw < value->prevraw && value->prevraw - newvalue.raw > 3)) {

        if (newvalue.t - value->t > 50) {
            value->prevraw = value->raw;
            value->raw = newvalue.raw;
            value->v = newvalue.raw / 5;
            value->t = newvalue.t;
        }
    }

    return value->v;
}

void display_off(volatile display_t *d)
{
    cli();

    // anodes off
    PORTC &= ~(_BV(PIN_ANODE_ONES) | _BV(PIN_ANODE_TENS));

    PORTB = 0xff;
    DDRC |= _BV(PIN_CATHODE_HUNDREDS_A) | _BV(PIN_CATHODE_HUNDREDS_B);

    d->segment = SEGMENT_A;
    d->on = 0;
    
    sei();
};

void display_on(volatile display_t *d)
{
    cli();
    d->on = 1;
    sei();
}

void display_toggle(volatile display_t *d)
{
    cli();
    d->on = !d->on;

    if (!d->on)
        display_off(d);
    sei();
}

void display_update(volatile display_t *d)
{
    const segment_t *s = &segment[d->segment];

    int hundreds = s->d == 2;
    int value = d->digits[s->d];

    PORTB = ~(digit[value] & s->port_b);

    if (hundreds && value == 1) {
        PORTC = (PORTC & 0b11110000) | s->port_c;
    } else {
        PORTC = (PORTC & 0b11111100) | s->port_c;
    }

    if (++d->segment == sizeof segment / sizeof segment[0])
        d->segment = 0;
}

void display_set(volatile display_t *d, int value)
{
    cli();

    d->digits[0] = value % 10;
    d->digits[1] = (value / 10) % 10;
    d->digits[2] = value >= 100;

    d->value = value;

    sei();
}

ISR(TIMER2_OVF_vect)
{
    // timer overflows every 400 micros

#ifdef DEBUG_TIMER2
    PORTD ^= _BV(PIN_LED_RED);
#endif

    TCNT2 = TIMER2_RESET_TO_400_MICROS;

    if (display.on)
        display_update(&display);
}

void settle_on_low(volatile uint8_t *port, uint8_t mask)
{
    long t = 0, t0 = 0;
    enum { SETTLE_ON_LOW_TIMEOUT_MS = 20 };

    loop_until_bit_is_clear(*port, mask);
    t0 = millis();

    // now wait for pin to stay low for at least x milliseconds
    while ((t = millis()) - t0 < SETTLE_ON_LOW_TIMEOUT_MS) {
	if (bit_is_set(*port, mask))
	    t0 = millis();
    }
}

void setup_int0(void)
{
    // set INT0 to trigger on falling edge
    EICRA |= _BV(ISC01);
    EICRA &= ~_BV(ISC00);
}

void enable_int0()
{
    EIMSK |= _BV(INT0);
}

void disable_int0()
{
    EIMSK &= ~_BV(INT0);
}

volatile long t0 = 0;

ISR(INT0_vect)
{
    disable_int0();
    t0 = micros();
}

int main(void)
{
    int pin = PIN_LED_BLUE;

    analogvalue_t delay;

    setup();
    setup_timer0();
    setup_int0();

    display_off(&display);
    setup_timer2();

    analog_init(&delay);
    getdelay(&delay);

    display_set(&display, delay.v);
    display_on(&display);

    if ((PIND & _BV(PIN_SW1)) == 0)
        pin = PIN_LED_RED;

    for (;;) {
        if ((PIND & _BV(PIN_SW1)) == 0) {
            long tmax = 1000L * delay.v;
            while (micros() - t0 < tmax)
                ;
            PORTD |= _BV(pin);
        } else {
            PORTD &= ~_BV(pin);
            enable_int0();
        }

        if ((PIND & _BV(PIN_SW2)) == 0) {
            settle_on_low(&PIND, PIN_SW2);
            loop_until_bit_is_set(PIND, PIN_SW2);
            display_toggle(&display);
        }

        getdelay(&delay);
        if (delay.v != display.value) {
            display_set(&display, delay.v);
        }
    }

    return 0;
}
