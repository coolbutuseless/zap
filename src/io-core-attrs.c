
#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include <Rversion.h>

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

#if R_VERSION < R_Version(4, 6, 0)
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
  if (Rf_isNull(cls_)) {
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


#else
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Determine the attributes on 'x_' and write them out
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_attrs(ctx_t *ctx, SEXP x_) {
  
  SEXP nms_ = PROTECT(R_getAttribNames(x_));
  size_t len = (size_t)Rf_length(nms_);
  write_len(ctx, len);
  if (len > 0) {
    write_sexp(ctx, nms_);
    for (size_t i = 0; i < len; i++) {
      write_sexp(ctx, Rf_getAttrib(x_, Rf_install(CHAR(STRING_ELT(nms_, i)))));
    }
  }
  UNPROTECT(1);

  // Write class
  SEXP cls_ = PROTECT(Rf_getAttrib(x_, R_ClassSymbol));
  if (Rf_isNull(cls_)) {
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

  size_t len = read_len(ctx);
  if (len > 0) {
    SEXP nms_ = PROTECT(read_sexp(ctx));
    for (size_t i = 0; i < len; i++) {
      SEXP val_ = PROTECT(read_sexp(ctx));
      SEXP nm_ = PROTECT(Rf_install(CHAR(STRING_ELT(nms_, i))));
      Rf_setAttrib(obj_, nm_, val_);
      UNPROTECT(2);
    }
    UNPROTECT(1);
  }
  
  // Read class
  SEXP cls_ = PROTECT(read_sexp(ctx));
  Rf_setAttrib(obj_, R_ClassSymbol, cls_);
  UNPROTECT(1);
}

#endif 

