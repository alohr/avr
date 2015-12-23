#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define DEBUG_TIMER1

enum {
    PIN_LED_BLUE = PB6,
    TIMER0_RESET_TO_400_MICROS = 256 - (F_CPU / 8 / 2500)
};


static const int DELAY = 500;

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

/*
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
*/

typedef struct {
    uint8_t digits[4];
    uint8_t digit;
    uint8_t segment;
    int value;
    int on;
} display_t;


static volatile display_t display;

static void display_update(volatile display_t *d)
{
    if (d->segment == 0) {
        // all segments off
        PORTA &= 0x01;

        // turn digit on
        uint8_t port_b = PORTB;
        port_b |= (_BV(PB6) | _BV(PB5) | _BV(PB4) | _BV(PB2));

        switch (d->digit) {
        case 0: port_b &= ~_BV(PB6); break;
        case 1: port_b &= ~_BV(PB5); break;
        case 2: port_b &= ~_BV(PB4); break;
        case 3: port_b &= ~_BV(PB2); break;
        }

        PORTB = port_b;
    }

    // segment
    int value = d->digits[d->digit];
    PORTA = (digit[value] & _BV(d->segment)) << 1;

    if (++d->segment == 7) {
        d->segment = 0;
        if (++d->digit == 4)
            d->digit = 0;
    }
}

static void display_on(volatile display_t *d)
{
    cli();
    d->on = 1;
    sei();
}

static void display_set(volatile display_t *d, int value)
{
    d->digits[0] = value % 10;
    d->digits[1] = (value / 10) % 10;
    d->digits[2] = (value / 100) % 10;
    d->digits[3] = (value / 1000) % 10;
    d->value = value;
}

static void setup(void)
{
    DDRA = 0xfe;
    PORTA = 0xfe;

    DDRB = _BV(PB6) | _BV(PB5) | _BV(PB4) | _BV(PB3) | _BV(PB2);
    PORTB |= _BV(PB6) | _BV(PB5) | _BV(PB4) | _BV(PB2);

    DDRB |= _BV(PB1);

    // Set ADC prescaler /8, 1 Mhz / 8 = 125 KHz which is inside
    // the desired 50-200 KHz range.


    ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0);
}

static void setup_timer0(void)
{
#if F_CPU == 1000000
    // prescale /8
    TCCR0A = 0;
    TCCR0B = _BV(CS01);
#elif F_CPU == 8000000
    // prescale /64
    TCCR0A = 0;
    TCCR0B = _BV(CS01) | _BV(CS00);
#else
#error F_CPU not recognized
#endif

    // overflow interrupt enable
    TIMSK |= _BV(TOIE0);
    TCNT0L = TIMER0_RESET_TO_400_MICROS;

    sei();
}

static inline void set10bit(volatile uint8_t *reg, int value)
{
    TC1H = (value >> 8) & 0x03;
    *reg = value & 0xff;
}

static void setup_timer1(void)
{
    // clear on compare match
    TCCR1A = _BV(COM1B1) | _BV(PWM1B);

    // prescale /4
    // running at 1 Mhz => every counter step = 4 microseconds
    TCCR1B = _BV(CS11) | _BV(CS10);

    // top
    set10bit(&OCR1C, 1000);

    // compare match
    // set10bit(&OCR1B, 125);

    // overflow interrupt enable
    // TIMSK |= _BV(TOIE1);
}

static void set_timer1_compare_match(int micros)
{
    set10bit(&OCR1B, micros / 4);
}

ISR(TIMER0_OVF_vect)
{
#ifdef DEBUG_TIMER0
    PORTB ^= _BV(PIN_LED_BLUE);
#endif
    // timer interrupt overflows every 400 microseconds
    TCNT0L = TIMER0_RESET_TO_400_MICROS;

    if (display.on)
        display_update(&display);
}

typedef struct {
    int raw, prevraw;
    int v;
} analogvalue_t;


void analog_init(analogvalue_t *value)
{
    value->raw = 0;
    value->prevraw = 0;
    value->v = 0;
}

int analog_read(int chan, analogvalue_t *value)
{
    uint8_t low, high;

    // VCC used as voltage reference
    ADMUX = (chan & 0x1f);

    // start single conversion
    ADCSRA |= _BV(ADSC);

    // wait for conversion to complete
    loop_until_bit_is_clear(ADCSRA, ADSC);

    low  = ADCL;
    high = ADCH;

    value->raw = (high << 8) | low;

    return value->raw;
}

int read_pot(analogvalue_t *value)
{
    analogvalue_t newvalue;
    enum { CHANNEL = 0, N_READINGS = 3, MICROSECONDS_OFFSET = 500 };

    int raw = 0;
    for (int i = 0; i < N_READINGS; i++) {
        analog_read(CHANNEL, &newvalue);
        raw += newvalue.raw;
    }

    newvalue.raw = raw / N_READINGS;

    if ((newvalue.raw > value->prevraw && newvalue.raw - value->prevraw > 3) ||
        (newvalue.raw < value->prevraw && value->prevraw - newvalue.raw > 3)) {

        value->prevraw = value->raw;
        value->raw = newvalue.raw;
        value->v = newvalue.raw * 2 + MICROSECONDS_OFFSET;
    }

    return value->v;
}

int main(void)
{
    int value = 0;
    analogvalue_t potvalue;

    setup();
    setup_timer0();
    setup_timer1();

    analog_init(&potvalue);
    read_pot(&potvalue);
    
    display_set(&display, value);
    display_on(&display);

    for (;;) {
        /* _delay_ms(2000); */
        /* potvalue.v++; */

        read_pot(&potvalue);
        if (potvalue.v != display.value) {
            display_set(&display, potvalue.v);
            set_timer1_compare_match(potvalue.v);
        }

        /* read_pot(&potvalue); */
        /* if (potvalue.v != display.value) { */
        /*     display_set(&display, potvalue.v); */
        /* } */
    }

    return 0;
}
