

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

#include "io-LGLSXP.h"
#include "utils-packing-1bit.h"

#define BUF_PACKED     0


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read/write un-transformed LGL values
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_LGLSXP_raw(ctx_t *ctx, SEXP x_) {
  write_uint8(ctx, LGLSXP);
  write_uint8(ctx, ZAP_LGL_RAW);
  size_t len = (size_t)Rf_length(x_);
  write_len(ctx, len);
  if (len == 0) return;
  ctx->write(ctx->user_data, LOGICAL(x_), len * 4);
}


SEXP read_LGLSXP_raw(ctx_t *ctx) {
  size_t len = read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(LGLSXP, (R_len_t)len));
  
  if (len > 0) {
    ctx->read(ctx->user_data, LOGICAL(obj_), len * 4);
  }  
  UNPROTECT(1);
  return obj_;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write logical
//
// In order to accommodate NA values, need at least 2-bits for each logical
// 
// Two possiblities:
//   (1) Encode each R logical in a single 2-bit value and pack
//   (2) Encode T/F in 1-bit value, then encode isNA in a 1-bit value.
//
// This code uses Method #2
//
// Method 2 is being used here as NAs are generally much less prevalent than
// actual T/F values in real data.
// So the vector of 1-bit "isNA" values will usually be mostly zeros and
// thus very very easy to compress.
//
// Using method 2 also simplifies the logic a little as there doesn't need
// to be an NA correction within the loop for every value.  And when 
// unpacking the values it is easy to recognise when a run of 32 logicals
// do not contain an NA value
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_LGLSXP_packed(ctx_t *ctx, SEXP x_) {
  
  // Header
  write_uint8(ctx, LGLSXP);
  write_uint8(ctx, ZAP_LGL_PACKED);
  size_t len = (size_t)Rf_xlength(x_);
  write_len(ctx, len);
  
  // Early return
  if (len == 0) return;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Pack T/F values into bitstream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t packed_len = pack_lgl(ctx, BUF_PACKED, x_);
  write_buf(ctx, BUF_PACKED, packed_len);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the auxilliary bitstream of NA locations
  // Note: NAs for logical are identical to NAs for integer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  pack_na_int(ctx, BUF_PACKED, x_);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Output
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_buf(ctx, BUF_PACKED, packed_len);
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read Logical values
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_LGLSXP_packed(ctx_t *ctx) {
  
  size_t len = read_len(ctx);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate an R logical vector.
  // This is initialised to all 0 by default
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP x_ = PROTECT(Rf_allocVector(LGLSXP, (R_xlen_t)len)); 
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Extract T/F values from bitstream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  memset(LOGICAL(x_), 0, len * sizeof(int32_t));
  read_buf(ctx, BUF_PACKED);
  unpack_lgl(ctx, BUF_PACKED, x_, len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set NA values using the auxilliary NA bistream
  // Note: NAs for logical is encoded the same as NA for integer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_PACKED);
  unpack_na_int(ctx, BUF_PACKED, x_, len);
  
  
  UNPROTECT(1);
  return x_;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_LGLSXP(ctx_t *ctx, SEXP x_) {
  
  if (Rf_length(x_) < ctx->opts->lgl_threshold) {
    write_LGLSXP_raw(ctx, x_);
    return;
  }
  
  switch(ctx->opts->lgl_transform) {
  case ZAP_LGL_RAW:
    write_LGLSXP_raw(ctx, x_);
    break;
  case ZAP_LGL_PACKED:
    write_LGLSXP_packed(ctx, x_);
    break;
  default:
    Rf_error("write_LGLSXP(): lgl transform not understood: %i", ctx->opts->lgl_transform);
  }
}

SEXP read_LGLSXP(ctx_t *ctx) {
  uint8_t method = read_uint8(ctx);
  switch(method) {
  case ZAP_LGL_RAW:
    return read_LGLSXP_raw(ctx);
    break;
  case ZAP_LGL_PACKED:
    return read_LGLSXP_packed(ctx);
    break;
  default:
    Rf_error("read_LGLSXP(): lgl transform not understood: %i", method);
  }
}













