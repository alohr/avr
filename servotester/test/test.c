#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t digits[4];
    uint8_t digit;
    uint8_t segment;
    int value;
    int on;
} display_t;

static void display_set(volatile display_t *d, int value)
{
    d->digits[0] = value % 10;
    d->digits[1] = (value / 10) % 10;
    d->digits[2] = (value / 100) % 10;
    d->digits[3] = (value / 1000) % 10;
    d->value = value;

    printf("%u %u %u %u\n",
           d->digits[3],
           d->digits[2],
           d->digits[1],
           d->digits[0]);
}

int main()
{
    display_t display;

    display_set(&display, 8888);

    return 0;
}

