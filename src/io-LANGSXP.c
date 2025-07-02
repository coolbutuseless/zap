
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
#include "io-core.h"
#include "io-LANGSXP.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write Language
//
// Write
//   - sexptype
//   - length
//   - for each elem:
//      - write elem
//      - write tag
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_LANGSXP(ctx_t *ctx, SEXP x_) {
  
  write_uint8(ctx, LANGSXP); // Write SEXP
  R_xlen_t len = Rf_xlength(x_);
  write_len(ctx, (uint64_t)len);
  
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP el_ = CAR(x_);
    write_sexp(ctx, el_);
    write_sexp(ctx, TAG(x_));
    x_ = CDR(x_);
  }
  
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read Language
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_LANGSXP(ctx_t *ctx) {
  
  R_xlen_t len = (R_xlen_t)read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(LANGSXP, len));
  SEXP p_ = obj_;
  
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP el_  = PROTECT(read_sexp(ctx));
    SEXP tag_ = PROTECT(read_sexp(ctx));
    SETCAR(p_, el_);
    SET_TAG(p_, tag_);
    p_ = CDR(p_);
    UNPROTECT(2);
  }
  
  UNPROTECT(1);
  return obj_;
}

