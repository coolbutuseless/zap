
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
#include "io-LISTSXP.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write pairlist
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_LISTSXP(ctx_t *ctx, SEXP x_) {
  
  write_uint8(ctx, LISTSXP);

  R_xlen_t len = Rf_xlength(x_);
  write_len(ctx, (uint64_t)len);
  
  for (SEXP cons_ = x_; cons_ != R_NilValue; cons_ = CDR(cons_)) {
    SEXP el_ = CAR(cons_);
    write_sexp(ctx, TAG(cons_));
    write_sexp(ctx, el_);
  }
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read pairlist
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_LISTSXP(ctx_t *ctx) {
  
  R_xlen_t len = (R_xlen_t)read_len(ctx);
  
  SEXP obj_ = PROTECT(Rf_allocVector(LISTSXP, len)); 
  SEXP p_ = obj_;
  
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP tag_ = PROTECT(read_sexp(ctx));
    SEXP el_  = PROTECT(read_sexp(ctx));
    SETCAR(p_, el_);
    SET_TAG(p_, tag_);
    UNPROTECT(2);
    if (i == (int32_t)len - 1) {
      // last value should point to R_NilValue to indicate end of list.
      SETCDR(p_, R_NilValue);
    } else {
      p_ = CDR(p_);
    }
  }
  
  UNPROTECT(1);
  return obj_;
}


