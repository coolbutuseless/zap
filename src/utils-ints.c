


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
#include "utils-ints.h"
#include "utils-zigzag.h"
#include "utils-shuffle.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This is a special case where a C uint32_t sequence needs to be saved
// to file - where the integer data did NOT come from a SEXP
//
// Currently used by:
//   - indexing patch data for ALP coding of REALSXP
//
// Future:
//    - use delta-frame encoding here?
//    - use zzshuf encoding here?
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



#define BUF_SHUF 4

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a vector of uint32_t to output using "delta + shuffle"
//
// @param buf_idx the buffer index containing the uint32_t data
// @param n_ints the number of integers in the buffer
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_uint32_buf(ctx_t *ctx, int buf_idx, size_t n_ints) {
  
  if (buf_idx == BUF_SHUF) {
    Rf_error("write_uint32_buf(): Conflicting buffers: %i", buf_idx);
  }
  
  write_len(ctx, n_ints);
  if (n_ints == 0) {
    return;
  }
  
  shuffle_delta4_buf_buf(ctx, buf_idx, BUF_SHUF, n_ints);
  write_buf(ctx, BUF_SHUF, n_ints * sizeof(uint32_t));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read uin32_t's from input, reverse "delta + shuffle" and place in 
// the nominated buffer
//
// @return number of uint32_ts in buffer
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t read_uint32_buf(ctx_t *ctx, int buf_idx) {
  
  if (buf_idx == BUF_SHUF) {
    Rf_error("read_uint32_buf(): Conflicting buffers: %i", buf_idx);
  }
  
  size_t n_ints = read_len(ctx);
  
  if (n_ints == 0) {
    return n_ints;
  }
  
  read_buf(ctx, BUF_SHUF);
  unshuffle_delta4_buf_buf(ctx, BUF_SHUF, buf_idx, n_ints);
  
  return n_ints;
}





