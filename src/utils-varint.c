


#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include "io-ctx.h"
#include "utils-varint.h"

#define MSB 0x80


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This is basicaly LEB128 variable length integer coding
// https://en.wikipedia.org/wiki/LEB128
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//             #            #       ##      #  
//                          #      #       ##  
//    #   #   ##    # ##   ####   #       # #  
//    #   #    #    ##  #   #     # ##   #  #  
//    #   #    #    #   #   #     ##  #  ##### 
//    #  ##    #    #   #   #  #  #   #     #  
//     ## #   ###   #   #    ##    ###      #  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_len(ctx_t *ctx, uint64_t val) {
  
  if (val < (1ULL << 7)) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f)      );
  } else if (val < (1ULL << ( 2 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f)      );
  } else if (val < (1ULL << (3 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f)      );
  } else if (val < (1ULL << (4 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 21) & 0x7f)      );
  } else if (val < (1ULL << (5 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 21) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 28) & 0x7f)      );
  } else if (val < (1ULL << (6 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 21) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 28) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 35) & 0x7f)      );
  } else if (val < (1ULL << (7 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 21) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 28) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 35) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 42) & 0x7f)      );
  } else if (val < (1ULL << (8 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 21) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 28) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 35) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 42) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 49) & 0x7f)      );
  } else if (val < (1ULL << (9 * 7))) {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 21) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 28) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 35) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 42) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 49) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 56) & 0x7f)      );
  } else {
    write_uint8(ctx, ((uint8_t)(val >>  0) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >>  7) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 14) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 21) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 28) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 35) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 42) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 49) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 56) & 0x7f) | MSB);
    write_uint8(ctx, ((uint8_t)(val >> 63) & 0x7f)      );
  }
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint64_t read_len(ctx_t *ctx) {
  
  uint64_t val = 0;
  uint8_t b = 0;
  int n = 0;
  do {
    b = read_uint8(ctx);
    val += (uint64_t)(b & 0x7f) << (7 * n);
    n++;
  } while (b & MSB);
  
  return val;
}





