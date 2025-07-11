
#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>


SEXP address_(SEXP x_) {
  char buf[128];
  snprintf(buf, 127, "%p", (void *)x_);
  
  return Rf_mkString(buf);
}

