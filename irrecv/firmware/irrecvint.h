/*
 * IRremote
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 */

#ifndef IRremoteint_h
#define IRremoteint_h

#define CLKFUDGE 3      // fudge factor for clock interrupt overhead
#define CLK 65536      	// max value for clock (timer1)
#define PRESCALE 8      // timer 0 clock prescale
#define SYSCLOCK F_CPU	// CPU clock frequency

#define CLKSPERUSEC (SYSCLOCK/PRESCALE/1000000)   // timer clocks per microsecond

#define ERR 0
#define DECODED 1

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// clock timer reset value
#define INIT_TIMER_COUNT1 (CLK - USECPERTICK * CLKSPERUSEC + CLKFUDGE)
#define RESET_TIMER1 TCNT1 = (uint16_t) (INIT_TIMER_COUNT1)

// pulse parameters in usec
#define NEC_HDR_MARK	9000
#define NEC_HDR_SPACE	4500
#define NEC_BIT_MARK	560
#define NEC_ONE_SPACE	1600
#define NEC_ZERO_SPACE	560
#define NEC_RPT_SPACE	2250

#define SONY_HDR_MARK	2400
#define SONY_HDR_SPACE	600
#define SONY_ONE_MARK	1200
#define SONY_ZERO_MARK	600
#define SONY_RPT_LENGTH 45000

#define RC5_T1		889
#define RC5_RPT_LENGTH	46000

#define RC6_HDR_MARK	2666
#define RC6_HDR_SPACE	889
#define RC6_T1		444
#define RC6_RPT_LENGTH	46000

#define JVC_HDR_MARK    8000
#define JVC_HDR_SPACE   4000
#define JVC_BIT_MARK    600
#define JVC_ONE_SPACE   1600
#define JVC_ZERO_SPACE  550
#define JVC_RPT_LENGTH  60000

#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__)
// percent tolerance in measurements
#define TOLERANCE 35
#else
#define TOLERANCE 25  
#endif

#ifdef IRRECV_FLOAT
#define LTOL (1.0 - TOLERANCE/100.) 
#define UTOL (1.0 + TOLERANCE/100.) 
#else
#define LTOL (100 - TOLERANCE)
#define UTOL (100 + TOLERANCE)
#endif

#define _GAP 5000 // Minimum map between transmissions
#define GAP_TICKS (_GAP/USECPERTICK)

#ifdef IRRECV_FLOAT
#define TICKS_LOW(us) (int) (((us) * LTOL/USECPERTICK))
#define TICKS_HIGH(us) (int) (((us) * UTOL/USECPERTICK + 1))
#else
#define TICKS_LOW(us) (int) (( (long) (us) * LTOL / (USECPERTICK * 100) ))
#define TICKS_HIGH(us) (int) (( (long) (us) * UTOL / (USECPERTICK * 100) + 1))
#endif

#define MATCH(measured_ticks, desired_us) \
    ((measured_ticks) >= TICKS_LOW(desired_us) && (measured_ticks) <= TICKS_HIGH(desired_us))

#define MATCH_MARK(measured_ticks, desired_us) \
    MATCH(measured_ticks, (desired_us) + MARK_EXCESS)

#define MATCH_SPACE(measured_ticks, desired_us) \
    MATCH((measured_ticks), (desired_us) - MARK_EXCESS)

// receiver states
#define STATE_IDLE     2
#define STATE_MARK     3
#define STATE_SPACE    4
#define STATE_STOP     5

// information for the interrupt handler
typedef struct {
  uint8_t rcvstate;          // state machine
  unsigned int timer;     // state timer, counts 50uS ticks.
  unsigned int rawbuf[RAWBUF]; // raw data
  uint8_t rawlen;         // counter of entries in rawbuf
} irparams_t;

// Defined in IRremote.cpp
extern volatile irparams_t irparams;

// IR detector output is active low
#define MARK  0
#define SPACE 1

#define TOPBIT 0x80000000

#define NEC_BITS  32
#define SONY_BITS 12
#define JVC_BITS  16

#define MIN_RC5_SAMPLES 11
#define MIN_RC6_SAMPLES 1

#endif

