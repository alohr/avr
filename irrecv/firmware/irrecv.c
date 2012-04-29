/*
 * IRremote
 * Version 0.11 August, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 */

#include <inttypes.h>
#include <avr/interrupt.h>

#include "irrecv.h"
#include "irrecvint.h"

volatile irparams_t irparams;

#define COMPILE_DECODE_NEC

#ifdef COMPILE_DECODE_NEC
static long decodeNEC(decode_results *results);
#endif
#ifdef COMPILE_DECODE_SONY
static long decodeSony(decode_results *results);
#endif
static int getRClevel(decode_results *results, int *offset, int *used, int t1);
static long decodeRC5(decode_results *results);
#ifdef COMPILE_DECODE_RC6
static long decodeRC6(decode_results *results);
#endif
#ifdef COMPILE_DECODE_JVC
static long decodeJVC(decode_results *results);
#endif

void setup_irrecv(uint8_t blinkflag)
{
  // set pin modes
#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__)
  sbi(DDRB, PB5); // IR activity indicator
#else
  sbi(DDRB, PB2); // IR activity indicator
#endif

  cbi(DDRB, PB4); // IR detector pin
  sbi(PORTB, PB4); // pull-up

  // initialize state machine variables
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;
  irparams.blinkflag = blinkflag;

#if F_CPU == 8000000 || F_CPU == 16000000
  // prescale /8
  TCCR1A = 0;
  TCCR1B = 0x02;
#elif F_CPU == 1000000
  // no prescaling
  TCCR1A = 0;
  TCCR1B = 0x01;
#else
#error Unknown F_CPU
#endif

  // Timer1 overflow interrupt enable
  sbi(TIMSK, TOIE1);

  RESET_TIMER1;

  sei();  // enable interrupts
}

// TIMER1 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 50 microseconds.
// rawlen counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a SPACE gets long, ready is set, state switches to IDLE, timing of SPACE continues.
// As soon as first MARK arrives, gap width is recorded, ready is cleared, and new logging starts

ISR(TIMER1_OVF_vect)
{
/*
#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__)
  PORTB |= _BV(PB5);
#else
  PORTB |= _BV(PB2);
#endif
*/

  RESET_TIMER1;

  uint8_t irdata = ((PINB & _BV(PB4)) != 0);

  irparams.timer++; // One more 50us tick
  if (irparams.rawlen >= RAWBUF) {
    // Buffer overflow
    irparams.rcvstate = STATE_STOP;
  }
  switch(irparams.rcvstate) {
  case STATE_IDLE: // In the middle of a gap
    if (irdata == MARK) {
      if (irparams.timer < GAP_TICKS) {
        // Not big enough to be a gap.
        irparams.timer = 0;
      } 
      else {
        // gap just ended, record duration and start recording transmission
        irparams.rawlen = 0;
        irparams.rawbuf[irparams.rawlen++] = irparams.timer;
        irparams.timer = 0;
        irparams.rcvstate = STATE_MARK;
      }
    }
    break;
  case STATE_MARK: // timing MARK
    if (irdata == SPACE) {   // MARK ended, record time
      irparams.rawbuf[irparams.rawlen++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_SPACE;
    }
    break;
  case STATE_SPACE: // timing SPACE
    if (irdata == MARK) { // SPACE just ended, record it
      irparams.rawbuf[irparams.rawlen++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_MARK;
    } 
    else { // SPACE
      if (irparams.timer > GAP_TICKS) {
        // big SPACE, indicates gap between codes
        // Mark current code as ready for processing
        // Switch to STOP
        // Don't reset timer; keep counting space width
        irparams.rcvstate = STATE_STOP;
      } 
    }
    break;
  case STATE_STOP: // waiting, measuring gap
    if (irdata == MARK) { // reset gap timer
      irparams.timer = 0;
    }
    break;
  }

  if (irparams.blinkflag) {
    if (irdata == MARK) {
      PORTB |= _BV(PB5);
    } else {
      PORTB &= ~(_BV(PB5));
    }
  }

/*
#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__)
  PORTB &= ~(_BV(PB5));
#else
  PORTB &= ~(_BV(PB2));
#endif
*/
}

void irrecv_resume(void)
{
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;
}

// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in results
int irrecv_decode(decode_results *results)
{
  results->rawbuf = irparams.rawbuf;
  results->rawlen = irparams.rawlen;

  if (irparams.rcvstate != STATE_STOP) {
    return ERR;
  }

#ifdef COMPILE_DECODE_NEC
  if (decodeNEC(results)) {
    return DECODED;
  }
#endif

#ifdef COMPILE_DECODE_SONY
  if (decodeSony(results)) {
    return DECODED;
  }
#endif

  if (decodeRC5(results)) {
    return DECODED;
  }

#ifdef COMPILE_DECODE_RC6
  if (decodeRC6(results)) {
    return DECODED;
  }
#endif

#ifdef COMPILE_DECODE_JVC
  if (decodeJVC(results)) {
    return DECODED;
  }
#endif

  if (results->rawlen >= 6) {
    // Only return raw buffer if at least 6 bits
    results->decode_type = UNKNOWN;
    results->bits = 0;
    results->value = 0;
    return DECODED;
  }

  // Throw away and start over
  irrecv_resume();
  return ERR;
}

#ifdef COMPILE_DECODE_NEC
static long decodeNEC(decode_results *results)
{
  long data = 0;
  int offset = 1; // Skip first space
  int i = 0;

  // Initial mark
  if (!MATCH_MARK(results->rawbuf[offset], NEC_HDR_MARK)) {
    return ERR;
  }
  offset++;
  // Check for repeat
  if (irparams.rawlen == 4 &&
    MATCH_SPACE(results->rawbuf[offset], NEC_RPT_SPACE) &&
    MATCH_MARK(results->rawbuf[offset+1], NEC_BIT_MARK)) {
    results->bits = 0;
    results->value = REPEAT;
    results->decode_type = NEC;
    return DECODED;
  }
  if (irparams.rawlen < 2 * NEC_BITS + 4) {
    return ERR;
  }
  // Initial space  
  if (!MATCH_SPACE(results->rawbuf[offset], NEC_HDR_SPACE)) {
    return ERR;
  }
  offset++;
  for (i = 0; i < NEC_BITS; i++) {
    if (!MATCH_MARK(results->rawbuf[offset], NEC_BIT_MARK)) {
      return ERR;
    }
    offset++;
    if (MATCH_SPACE(results->rawbuf[offset], NEC_ONE_SPACE)) {
      data = (data << 1) | 1;
    } 
    else if (MATCH_SPACE(results->rawbuf[offset], NEC_ZERO_SPACE)) {
      data <<= 1;
    } 
    else {
      return ERR;
    }
    offset++;
  }
  // Success
  results->bits = NEC_BITS;
  results->value = data;
  results->decode_type = NEC;
  return DECODED;
}
#endif

#ifdef COMPILE_DECODE_SONY
static long decodeSony(decode_results *results)
{
  long data = 0;
  if (irparams.rawlen < 2 * SONY_BITS + 2) {
    return ERR;
  }
  int offset = 1; // Skip first space
  // Initial mark
  if (!MATCH_MARK(results->rawbuf[offset], SONY_HDR_MARK)) {
    return ERR;
  }
  offset++;

  while (offset + 1 < irparams.rawlen) {
    if (!MATCH_SPACE(results->rawbuf[offset], SONY_HDR_SPACE)) {
      break;
    }
    offset++;
    if (MATCH_MARK(results->rawbuf[offset], SONY_ONE_MARK)) {
      data = (data << 1) | 1;
    } 
    else if (MATCH_MARK(results->rawbuf[offset], SONY_ZERO_MARK)) {
      data <<= 1;
    } 
    else {
      return ERR;
    }
    offset++;
  }

  // Success
  results->bits = (offset - 1) / 2;
  if (results->bits < 12) {
    results->bits = 0;
    return ERR;
  }
  results->value = data;
  results->decode_type = SONY;
  return DECODED;
}
#endif

// Gets one undecoded level at a time from the raw buffer.
// The RC5/6 decoding is easier if the data is broken into time intervals.
// E.g. if the buffer has MARK for 2 time intervals and SPACE for 1,
// successive calls to getRClevel will return MARK, MARK, SPACE.
// offset and used are updated to keep track of the current position.
// t1 is the time interval for a single bit in microseconds.
// Returns -1 for error (measured time interval is not a multiple of t1).

static int getRClevel(decode_results *results, int *offset, int *used, int t1) {
  if (*offset >= results->rawlen) {
    // After end of recorded buffer, assume SPACE.
    return SPACE;
  }
  int width = results->rawbuf[*offset];
  int val = ((*offset) % 2) ? MARK : SPACE;
  int correction = (val == MARK) ? MARK_EXCESS : - MARK_EXCESS;

  int avail;
  if (MATCH(width, t1 + correction)) {
    avail = 1;
  } 
  else if (MATCH(width, 2*t1 + correction)) {
    avail = 2;
  } 
  else if (MATCH(width, 3*t1 + correction)) {
    avail = 3;
  } 
  else {
    return -1;
  }

  (*used)++;
  if (*used >= avail) {
    *used = 0;
    (*offset)++;
  }

  return val;   
}

static long decodeRC5(decode_results *results) {
  if (irparams.rawlen < MIN_RC5_SAMPLES + 2) {
    return ERR;
  }
  int offset = 1; // Skip gap space
  long data = 0;
  int used = 0;
  // Get start bits
  if (getRClevel(results, &offset, &used, RC5_T1) != MARK) return ERR;
  if (getRClevel(results, &offset, &used, RC5_T1) != SPACE) return ERR;
  if (getRClevel(results, &offset, &used, RC5_T1) != MARK) return ERR;
  int nbits;
  for (nbits = 0; offset < irparams.rawlen; nbits++) {
    int levelA = getRClevel(results, &offset, &used, RC5_T1); 
    int levelB = getRClevel(results, &offset, &used, RC5_T1);
    if (levelA == SPACE && levelB == MARK) {
      // 1 bit
      data = (data << 1) | 1;
    } 
    else if (levelA == MARK && levelB == SPACE) {
      // zero bit
      data <<= 1;
    } 
    else {
      return ERR;
    } 
  }

  // Success
  results->bits = nbits;
  if (results->value == data)
    results->value = REPEAT;
  else
    results->value = data;
  results->decode_type = RC5;
  return DECODED;
}

#ifdef COMPILE_DECODE_RC6
static long decodeRC6(decode_results *results) {
  if (results->rawlen < MIN_RC6_SAMPLES) {
    return ERR;
  }
  int offset = 1; // Skip first space
  // Initial mark
  if (!MATCH_MARK(results->rawbuf[offset], RC6_HDR_MARK)) {
    return ERR;
  }
  offset++;
  if (!MATCH_SPACE(results->rawbuf[offset], RC6_HDR_SPACE)) {
    return ERR;
  }
  offset++;
  long data = 0;
  int used = 0;
  // Get start bit (1)
  if (getRClevel(results, &offset, &used, RC6_T1) != MARK) return ERR;
  if (getRClevel(results, &offset, &used, RC6_T1) != SPACE) return ERR;
  int nbits;
  for (nbits = 0; offset < results->rawlen; nbits++) {
    int levelA, levelB; // Next two levels
    levelA = getRClevel(results, &offset, &used, RC6_T1); 
    if (nbits == 3) {
      // T bit is double wide; make sure second half matches
      if (levelA != getRClevel(results, &offset, &used, RC6_T1)) return ERR;
    } 
    levelB = getRClevel(results, &offset, &used, RC6_T1);
    if (nbits == 3) {
      // T bit is double wide; make sure second half matches
      if (levelB != getRClevel(results, &offset, &used, RC6_T1)) return ERR;
    } 
    if (levelA == MARK && levelB == SPACE) { // reversed compared to RC5
      // 1 bit
      data = (data << 1) | 1;
    } 
    else if (levelA == SPACE && levelB == MARK) {
      // zero bit
      data <<= 1;
    } 
    else {
      return ERR; // Error
    } 
  }
  // Success
  results->bits = nbits;
  results->value = data;
  results->decode_type = RC6;
  return DECODED;
}
#endif

#ifdef COMPILE_DECODE_JVC
static long decodeJVC(decode_results *results) {
  long data = 0;
  int offset = 1; // Skip first space
  int i = 0;

  // Check for repeat
  if (irparams.rawlen - 1 == 33 &&
      MATCH_MARK(results->rawbuf[offset], JVC_BIT_MARK) &&
      MATCH_MARK(results->rawbuf[irparams.rawlen-1], JVC_BIT_MARK)) {
    results->bits = 0;
    results->value = REPEAT;
    results->decode_type = JVC;
    return DECODED;
  } 

  // Initial mark
  if (!MATCH_MARK(results->rawbuf[offset], JVC_HDR_MARK)) {
    return ERR;
  }
  offset++; 
  if (irparams.rawlen < 2 * JVC_BITS + 1 ) {
    return ERR;
  }
  // Initial space 
  if (!MATCH_SPACE(results->rawbuf[offset], JVC_HDR_SPACE)) {
    return ERR;
  }
  offset++;
  for (i = 0; i < JVC_BITS; i++) {
    if (!MATCH_MARK(results->rawbuf[offset], JVC_BIT_MARK)) {
      return ERR;
    }
    offset++;
    if (MATCH_SPACE(results->rawbuf[offset], JVC_ONE_SPACE)) {
      data = (data << 1) | 1;
    } 
    else if (MATCH_SPACE(results->rawbuf[offset], JVC_ZERO_SPACE)) {
      data <<= 1;
    } 
    else {
      return ERR;
    }
    offset++;
  }
  // Stop bit
  if (!MATCH_MARK(results->rawbuf[offset], JVC_BIT_MARK)){
    return ERR;
  }
  // Success
  results->bits = JVC_BITS;
  results->value = data;
  results->decode_type = JVC;
  return DECODED;
}
#endif

/*
 * Local variables:
 * mode: c
 * c-basic-offset: 2
 * End:
 */
