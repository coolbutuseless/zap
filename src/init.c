
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "io-ctx.h"

extern SEXP zap_version_(void);

extern SEXP write_zap_(SEXP obj_, SEXP filename_, SEXP opts_) ;
extern SEXP read_zap_(SEXP filename_, SEXP opts_);

extern SEXP zap_count_(SEXP x_, SEXP opts_);
  
static const R_CallMethodDef CEntries[] = {
  
  {"zap_version_", (DL_FUNC) &zap_version_, 0},
  
  {"write_zap_"  , (DL_FUNC) &write_zap_  , 3},
  {"read_zap_"   , (DL_FUNC) &read_zap_   , 2},
  
  {"zap_count_", (DL_FUNC) &zap_count_, 2},
  
  {NULL , NULL, 0}
};



void R_init_zap(DllInfo *info) {
  R_registerRoutines(
    info,      // DllInfo
    NULL,      // .C
    CEntries,  // .Call
    NULL,      // Fortran
    NULL       // External
  );
  R_useDynamicSymbols(info, FALSE);
}



