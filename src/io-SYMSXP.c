
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
#include "io-SYMSXP.h"


#define BUF_SYM 0


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a symbol
// A symbol is just a character string.
// An empty string is used to indicate a missing argument.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_SYMSXP(ctx_t *ctx, SEXP x_) {
  write_uint8(ctx, SYMSXP);
  
  const char *chr = CHAR(PRINTNAME(x_));
  unsigned long len = strlen(chr) + 1;  // NUL-terminated
  write_ptr(ctx, (void *)chr, len);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read a symbol
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_SYMSXP(ctx_t *ctx) {
  
  read_buf(ctx, BUF_SYM);
  const char *c = (const char *)ctx->buf[BUF_SYM];
  
  if (strlen(c) == 0) {
    return R_MissingArg;
  }
  
  return Rf_install(c);
}

