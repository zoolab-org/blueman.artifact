#ifndef __MUTATE_STRATEGY_H_
#define __MUTATE_STRATEGY_H_
#include <stdint.h>
#include "utils.h"

void mutate_random_byte(uint8_t*, uint32_t);
void mutate_random_bytes_insert(uint8_t*, uint32_t, uint32_t, uint8_t);
void mutate_arithmetic(uint8_t*, uint32_t, uint32_t, uint8_t);
void mutate_bits_flip(uint8_t*, uint32_t, uint32_t, uint8_t);
void mutate_rand_bits(uint8_t*, uint32_t, uint32_t, uint8_t);
void mutate_interest(uint8_t*, uint32_t, uint32_t, uint8_t);
void mutate_havoc(uint8_t*, uint32_t, uint32_t);

#define ARITH_MAX 50
#define INTERESTING_8 \
  -128,          /* Overflow signed 8-bit when decremented  */ \
  -1,            /*                                         */ \
   0,            /*                                         */ \
   1,            /*                                         */ \
   16,           /* One-off with common buffer size         */ \
   32,           /* One-off with common buffer size         */ \
   64,           /* One-off with common buffer size         */ \
   100,          /* One-off with common buffer size         */ \
   127           /* Overflow signed 8-bit when incremented  */

#define INTERESTING_16 \
  -32768,        /* Overflow signed 16-bit when decremented */ \
  -129,          /* Overflow signed 8-bit                   */ \
   128,          /* Overflow signed 8-bit                   */ \
   255,          /* Overflow unsig 8-bit when incremented   */ \
   256,          /* Overflow unsig 8-bit                    */ \
   512,          /* One-off with common buffer size         */ \
   1000,         /* One-off with common buffer size         */ \
   1024,         /* One-off with common buffer size         */ \
   4096,         /* One-off with common buffer size         */ \
   32767         /* Overflow signed 16-bit when incremented */
#define INTERESTING_32 \
  -2147483648LL, /* Overflow signed 32-bit when decremented */ \
  -100663046,    /* Large negative number (endian-agnostic) */ \
  -32769,        /* Overflow signed 16-bit                  */ \
   32768,        /* Overflow signed 16-bit                  */ \
   65535,        /* Overflow unsig 16-bit when incremented  */ \
   65536,        /* Overflow unsig 16 bit                   */ \
   100663045,    /* Large positive number (endian-agnostic) */ \
   2147483647    /* Overflow signed 32-bit when incremented */

#endif
