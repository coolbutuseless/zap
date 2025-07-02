
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


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write CLOSXP
//
// "A closure is a function with data." John D. Cock.
// Closures include the environment of the parent function and can access
// variables there.
// In general, all R functions are closures. (excluding primitive functions)
//
// A closure = formal arguments + body of function + an environment
// There may also be a TAG set.
//
// Write:
//   - sexptype (int)
//   - formals (sexp)
//   - body (sexp)
//   - env  (sexp)
//   - tag (sexp)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_CLOSXP(ctx_t *ctx, SEXP x_) {
  write_uint8(ctx, CLOSXP);
  
  write_sexp(ctx, R_ClosureFormals(x_));
  write_sexp(ctx, R_ClosureBody(x_));
  write_sexp(ctx, R_ClosureEnv(x_));
  write_sexp(ctx, TAG(x_));
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read CLOSXP
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_CLOSXP(ctx_t *ctx) {
  
  SEXP formals_ = PROTECT(read_sexp(ctx));
  SEXP body_    = PROTECT(read_sexp(ctx));
  SEXP cloenv_  = PROTECT(read_sexp(ctx));
  SEXP tag_     = PROTECT(read_sexp(ctx));
  
  SEXP x_ = PROTECT(R_mkClosure(formals_, body_, cloenv_));
  SET_TAG(x_, tag_);
  
  UNPROTECT(5);
  return x_;
}

