
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
#include "io-RAWSXP.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a raw vector
//
// Write
//   - sexptype
//   - length
//   - bytes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_RAWSXP(ctx_t *ctx, SEXP x_) {
  write_uint8(ctx, RAWSXP);
  size_t len = (size_t)Rf_xlength(x_);
  write_len(ctx, len);
  ctx->write(ctx->user_data, RAW(x_), len);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read raw vector
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_RAWSXP(ctx_t *ctx) {
  size_t len = read_len(ctx);
  SEXP x_ = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)len)); 
  
  if (len > 0) {
    ctx->read(ctx->user_data, RAW(x_), len);
  }
  
  UNPROTECT(1);
  return x_;
}



