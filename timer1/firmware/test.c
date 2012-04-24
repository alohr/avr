#include <stdio.h>

#define F_CPU 16000000L

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( ((a) * 1000L) / (F_CPU / 1000L) )
#define microsecondsToClockCycles(a) ( ((a) * (F_CPU / 1000L)) / 1000L )

#define MICROSECONDS_PER_TIMER1_OVERFLOW (clockCyclesToMicroseconds(8 * 65536))

int main(void)
{
    printf("MICROSECONDS_PER_TIMER1_OVERFLOW = %ld\n",
  	    MICROSECONDS_PER_TIMER1_OVERFLOW);

    return 0;
}
