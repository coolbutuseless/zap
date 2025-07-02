


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



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  Logical T/F and "is.na()" can be stored in 1-bit-per-value for:
//    - STRSEXP
//    - INTSXP
//    - LGLSXP
//
// Note: these functions are all pretty much identical and could 
// be turned into a MACRO.  Complicating the code with macros doesn't feel worth it.
//
// Future:
//   is this style of NA coding worthwhile for any of the REALSXP transforms?
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Allocate 1-bit per element in 'x' and use it to indicate if value
// at this location is NA
//
// @param ctx zap context
// @param BUF_IDX which buffer to use to hold result
// @param x_ STRSXP object
//
// @return nbytes in bitstream written to output buffer
//         Data is in ctx->buf[BUF_IDX]
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t pack_na_str(ctx_t *ctx, int BUF_IDX, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // len              - how many original values
  // n_container_ints - how many uint32s are needed to hold the NA bitstream?
  // packed_len       - how many bytes are needed to hold the NA bitstream?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t len              = (size_t)Rf_length(x_);
  size_t n_container_ints = (size_t)ceil((double)len/(8.0 * sizeof(uint32_t)));
  size_t packed_len       = n_container_ints * sizeof(uint32_t);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Ensure the nominated buffer has enough room to store the bitstream 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  prepare_buf(ctx, BUF_IDX, packed_len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get a (uint32_t *) into the buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint32_t *nap = (uint32_t *)ctx->buf[BUF_IDX];
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // In groups of 32
  //   is this an NA string?
  //   add the boolean bit to the current int
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const SEXP *x = STRING_PTR_RO(x_);
  
  int i = 0;
  for (; i < (int64_t)len - (32 - 1); i += 32) {
    *nap = 0;
    for (int j = 0; j < 32; j++) {
      *nap |= (uint32_t)(x[i + j] == NA_STRING) << j;
    }
    nap++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // The input vector (x_) is unlikely to be an exact multiple of 32, 
  // so there are leftovers to process
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (i < len) {
    *nap = 0;
    int j = 0;
    for (; i < len; i++) {
      *nap |= (uint32_t)(x[i] == NA_STRING) << j;
      j++;
    }
  }
  
  
  return packed_len;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a binary bitstream to set NA values in x_
//
// @param ctx zap context
// @param BUF_IDX integer. Which buffer to use
// @param len number of elements to unpack
//
// @return None.  x_ modified in-place
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void unpack_na_str(ctx_t *ctx, int BUF_IDX, SEXP x_, size_t len) {
  
  uint32_t *nap = (uint32_t *)ctx->buf[BUF_IDX];
  int i = 0;
  for (; i < (int32_t)len - (32 - 1); i += 32) {
    if (*nap != 0) {
      for (int j = 0; j < 32; j++) {
        if ((*nap >> j) & 0x01) SET_STRING_ELT(x_, i + j, NA_STRING);
      }
    }
    nap++;
  }
  
  // remainder
  int j = 0;
  for (; i < len; i++) {
    if ((*nap >> j) & 0x01) SET_STRING_ELT(x_, i, NA_STRING);
    j++;
  }
  
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Allocate 1-bit per element in 'x' and use it to indicate if value
// at this location is NA
//
// @param ctx zap context
// @param BUF_IDX which buffer to use to hold result
// @param x_ INTSXP object
//
// @return nbytes in bitstream written to output buffer
//         Data is in ctx->buf[BUF_IDX]
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t pack_na_int(ctx_t *ctx, int BUF_IDX, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // len              - how many original values
  // n_container_ints - how many uint32s are needed to hold the NA bitstream?
  // packed_len       - how many bytes are needed to hold the NA bitstream?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t len              = (size_t)Rf_length(x_);
  size_t n_container_ints = (size_t)ceil((double)len/(8.0 * sizeof(uint32_t)));
  size_t packed_len       = n_container_ints * sizeof(uint32_t);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Ensure the nominated buffer has enough room to store the bitstream 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  prepare_buf(ctx, BUF_IDX, packed_len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get a (uint32_t *) into the buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint32_t *nap = (uint32_t *)ctx->buf[BUF_IDX];
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // In groups of 32
  //   is this an NA string?
  //   add the boolean bit to the current int
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int32_t *x = INTEGER(x_);
  int i = 0;
  for (; i < (int64_t)len - (32 - 1); i += 32) {
    *nap = 0;
    for (int j = 0; j < 32; j++) {
      *nap |= (uint32_t)(x[i + j] == NA_INTEGER) << j;
    }
    nap++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // The input vector (x_) is unlikely to be an exact multiple of 32, 
  // so there are leftovers to process
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (i < len) {
    *nap = 0;
    int j = 0;
    for (; i < len; i++) {
      *nap |= (uint32_t)(x[i] == NA_INTEGER) << j;
      j++;
    }
  }
  
  
  return packed_len;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a binary bitstream to set NA values in x_
//
// @param ctx zap context
// @param BUF_IDX integer. Which buffer to use
// @param len number of elements to unpack
//
// @return None.  x_ modified in-place
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void unpack_na_int(ctx_t *ctx, int BUF_IDX, SEXP x_, size_t len) {
  
  int32_t *x = INTEGER(x_);
  
  uint32_t *nap = (uint32_t *)ctx->buf[BUF_IDX];
  int i = 0;
  for (; i < (int32_t)len - (32 - 1); i += 32) {
    if (*nap != 0) {
      for (int j = 0; j < 32; j++) {
        if ((*nap >> j) & 0x01) x[i + j] = NA_INTEGER;
      }
    }
    nap++;
  }
  
  // remainder
  int j = 0;
  for (; i < len; i++) {
    if ((*nap >> j) & 0x01) x[i] = NA_INTEGER;
    j++;
  }
  
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Allocate 1-bit per element in 'x' and use it to indicate if value
// at this location is true
//
// @param ctx zap context
// @param BUF_IDX which buffer to use to hold result
// @param x_ LGLSXP object
//
// @return nbytes in bitstream written to output buffer
//         Data is in ctx->buf[BUF_IDX]
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t pack_lgl(ctx_t *ctx, int BUF_IDX, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // len              - how many original values
  // n_container_ints - how many uint32s are needed to hold the NA bitstream?
  // packed_len       - how many bytes are needed to hold the NA bitstream?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t len              = (size_t)Rf_length(x_);
  size_t n_container_ints = (size_t)ceil((double)len/(8.0 * sizeof(uint32_t)));
  size_t packed_len       = n_container_ints * sizeof(uint32_t);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Ensure the nominated buffer has enough room to store the bitstream 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  prepare_buf(ctx, BUF_IDX, packed_len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get a (uint32_t *) into the buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint32_t *nap = (uint32_t *)ctx->buf[BUF_IDX];
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // In groups of 32
  //   is this an NA string?
  //   add the boolean bit to the current int
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int32_t *x = LOGICAL(x_);
  int i = 0;
  for (; i < (int64_t)len - (32 - 1); i += 32) {
    *nap = 0;
    for (int j = 0; j < 32; j++) {
      *nap |= (uint32_t)(x[i + j] & 0x01) << j;
    }
    nap++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // The input vector (x_) is unlikely to be an exact multiple of 32, 
  // so there are leftovers to process
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (i < len) {
    *nap = 0;
    int j = 0;
    for (; i < len; i++) {
      *nap |= (uint32_t)(x[i] & 0x01) << j;
      j++;
    }
  }
  
  
  return packed_len;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a binary bitstream to set logical values in x_
//
// @param ctx zap context
// @param BUF_IDX integer. Which buffer to use
// @param len number of elements to unpack
//
// @return None.  x_ modified in-place
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void unpack_lgl(ctx_t *ctx, int BUF_IDX, SEXP x_, size_t len) {
  
  int32_t *x = LOGICAL(x_);
  
  uint32_t *nap = (uint32_t *)ctx->buf[BUF_IDX];
  int i = 0;
  for (; i < (int32_t)len - (32 - 1); i += 32) {
    if (*nap != 0) {
      for (int j = 0; j < 32; j++) {
        x[i + j] = (*nap >> j) & 0x01;
      }
    }
    nap++;
  }
  
  // remainder
  int j = 0;
  for (; i < len; i++) {
    x[i] = (*nap >> j) & 0x01;
    j++;
  }
  
}




