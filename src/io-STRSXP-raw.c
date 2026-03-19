

#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <zlib.h>

#include "io-ctx.h"
#include "io-STRSXP.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ####                
//   #   #               
//   #   #   ###   #   # 
//   ####       #  #   # 
//   # #     ####  # # # 
//   #  #   #   #  # # # 
//   #   #   ####   # #  
//
// Read/write raw untransformed strings.
// i.e. Sequence of (strlen, string) objects
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUF_CHR 0

void write_STRSXP_raw(ctx_t *ctx, SEXP x_) {
  
  write_uint8(ctx, STRSXP);
  write_uint8(ctx, ZAP_STR_RAW);
  
  R_xlen_t len = Rf_xlength(x_);
  write_len(ctx, (uint64_t)len);
  
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP chr_ = STRING_ELT(x_, i);
    int is_na = (chr_ == NA_STRING);
    write_uint8(ctx, (uint8_t)is_na);
    if (!is_na) {
      write_ptr(ctx, (void *)CHAR(chr_), (size_t)Rf_length(chr_));  
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_STRSXP_raw(ctx_t *ctx) {
  
  R_xlen_t len = (R_xlen_t)read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(STRSXP, len)); 
  
  for (R_xlen_t i = 0; i < len; i++) {
    int is_na = read_uint8(ctx);
    if (is_na) {
      SET_STRING_ELT(obj_, i, NA_STRING);
    } else {
      size_t slen = read_buf(ctx, BUF_CHR);
      SET_STRING_ELT(obj_, i, Rf_mkCharLen((const char *)ctx->buf[BUF_CHR], (int)slen));
    }
  }
  
  UNPROTECT(1);
  return obj_;
}

#undef BUF_CHR

