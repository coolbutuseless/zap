
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
#include "io-CLOSXP.h"
#include "io-ENVSXP.h"
#include "io-EXPRSXP.h"
#include "io-INTSXP.h"
#include "io-LANGSXP.h"
#include "io-LGLSXP.h"
#include "io-LISTSXP.h"
#include "io-RAWSXP.h"
#include "io-REALSXP.h"
#include "io-STRSXP.h"
#include "io-SYMSXP.h"
#include "io-VECSXP.h"

#include "io-factor.h"
#include "io-serialize.h"
#include "io-core-attrs.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Determine the attributes on 'x_' and write them out
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_attrs(ctx_t *ctx, SEXP x_) {
  
  // Write attributes. This is either a LISTSXP (pairlist) or NULL
  SEXP attrs_ = PROTECT(ATTRIB(x_));
  if (Rf_isNull(attrs_)) {
    write_uint8(ctx, NILSXP);
  } else {
    write_sexp(ctx, attrs_);
  }
  UNPROTECT(1);
  
  // // Write names
  // SEXP nms_ = PROTECT(Rf_getAttrib(x_, R_NamesSymbol));
  // write_sexp(ctx, nms_);
  // UNPROTECT(1);
  
  // Write class
  SEXP cls_ = PROTECT(Rf_getAttrib(x_, R_ClassSymbol));
  if (Rf_isNull(attrs_)) {
    write_uint8(ctx, NILSXP);
  } else {
    write_sexp(ctx, cls_);
  }
  UNPROTECT(1);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read attributes and assign them onto 'obj_'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_attrs(ctx_t *ctx, SEXP obj_) {
  // read attributes
  SEXP attrs_ = PROTECT(read_sexp(ctx));
  SET_ATTRIB(obj_, attrs_);
  UNPROTECT(1);
  
  // // Read names
  // SEXP nms_ = PROTECT(read_sexp(ctx));
  // Rf_setAttrib(obj_, R_NamesSymbol, nms_);
  // UNPROTECT(1);
  
  // Read class
  SEXP cls_ = PROTECT(read_sexp(ctx));
  Rf_setAttrib(obj_, R_ClassSymbol, cls_);
  UNPROTECT(1);
}




