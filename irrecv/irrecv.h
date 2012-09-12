#ifndef IRRECV_H
#define IRRECV_H

#undef COMPILE_DECODE_NEC  
#undef COMPILE_DECODE_SONY
#undef COMPILE_DECODE_RC6
#undef COMPILE_DECODE_JVC

// Length of raw duration buffer
#if defined(COMPILE_DECODE_NEC)  || \
    defined(COMPILE_DECODE_SONY) || \
    defined(COMPILE_DECODE_RC6)  || \
    defined(COMPILE_DECODE_JCV)
#define RAWBUF 68
#else 
// RC5 only
#define RAWBUF 24 
#endif

// Results returned from the decoder
typedef struct {
  int decode_type; // NEC, SONY, RC5, UNKNOWN
  unsigned long value; // Decoded value
  int bits; // Number of bits in decoded value
  volatile unsigned int *rawbuf; // Raw intervals in .5 us ticks
  int rawlen; // Number of records in rawbuf.
} decode_results;

// Values for decode_type
#define NEC 1
#define SONY 2
#define RC5 3
#define RC6 4
#define JVC 5
#define UNKNOWN -1

// Decoded value for NEC when a repeat code is received
#define REPEAT 0xffffffff

void setup_irrecv(void);

int irrecv_decode(decode_results *results);
void irrecv_resume(void);

// Some useful constants
#define USECPERTICK 50  // microseconds per clock interrupt tick

// Marks tend to be 100us too long, and spaces 100us too short
// when received due to sensor lag.
#define MARK_EXCESS 100

#endif /* IRRECV_H */
