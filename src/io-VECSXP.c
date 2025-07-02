
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
#include "io-VECSXP.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a stnadard R list
//
// Write:
//   sexptype
//   length
//   for each elem:
//      write elem
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_VECSXP(ctx_t *ctx, SEXP x_) {
  
  write_uint8(ctx, VECSXP);
  R_xlen_t len = Rf_xlength(x_);
  write_len(ctx, (uint64_t)len);
  
  for (R_xlen_t i = 0; i < len; i++) {
    write_sexp(ctx, VECTOR_ELT(x_, i));
  }
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read a standard R list
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_VECSXP(ctx_t *ctx) {
  
  R_xlen_t len = (R_xlen_t)read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(VECSXP, len)); 
  
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP el_ = PROTECT(read_sexp(ctx));
    SET_VECTOR_ELT(obj_, i, el_);
    UNPROTECT(1);
  }
  
  UNPROTECT(1);
  return obj_;
}

