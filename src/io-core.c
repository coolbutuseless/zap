
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

#include "utils-df.h"

//   0	NILSXP	NULL ------------------------ Yes
//   1	SYMSXP	symbols --------------------- Yes
//   2	LISTSXP	pairlists ------------------- Yes                   
//   3	CLOSXP	closures -------------------- Yes
//   4	ENVSXP	environments ---------------- Yes
//   5	PROMSXP	promises -------------------- 
//   6	LANGSXP	language objects ------------ Yes
//   7	SPECIALSXP	special functions -------
//   8	BUILTINSXP	builtin functions -------
//   9	CHARSXP	internal character strings -- Not needed?
//  10	LGLSXP	logical vectors ------------- Yes
//  11
//  12
//  13	INTSXP	integer vectors ------------- Yes
//  14	REALSXP	numeric vectors ------------- Yes
//  15	CPLXSXP	complex vectors ------------- Yes
//  16	STRSXP	character vectors ----------- Yes
//  17	DOTSXP	dot-dot-dot object ----------
//  18	ANYSXP	make “any” args work --------
//  19	VECSXP	list (generic vector) ------- Yes
//  20	EXPRSXP	expression vector ----------- Partial: SYMSXP, LANGSXP
//  21	BCODESXP	byte code -----------------
//  22	EXTPTRSXP	external pointer ----------
//  23	WEAKREFSXP	weak reference ----------
//  24	RAWSXP	raw vector ------------------ Yes
//  25	S4SXP	S4 classes not of simple type - 



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Retrieve the ZAP_VERSION
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP zap_version_(void) {
  return Rf_ScalarInteger(ZAP_VERSION);
}


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


// from ctx.c
extern char *sexp_nms[32];

extern size_t get_position(void *user_data);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write any SEXP object 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_sexp(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Track objects 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  bool used_rserialize = false;
  int start;
  int obj_count;
  
  if (ctx->opts->verbosity & ZAP_VERBOSITY_OBJDF) {
    ctx->depth++;
    start = (int)get_position(ctx->user_data);
    obj_count = (int)ctx->obj_count++;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For ALTREP just serialize the whole object
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ALTREP(x_)) {
    write_rserialize(ctx, x_);
    
    ctx->depth--;
    return;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Most things have attributes.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  bool possibly_has_attrs = true;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decide on how to write this object - which type-specific function to call?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  switch(TYPEOF(x_)) {
  case NILSXP: 
    write_uint8(ctx, NILSXP);
    possibly_has_attrs = false;
    break;
  case LGLSXP:
    write_LGLSXP(ctx, x_);
    break;
  case REALSXP:
    write_REALSXP(ctx, x_, false);
    break;
  case INTSXP:
    if (Rf_isFactor(x_)) {
      write_factor(ctx, x_);
    } else {
      write_INTSXP(ctx, x_);
    }
    break;
  case CPLXSXP:
    write_REALSXP(ctx, x_, true);
    break;
  case RAWSXP:
    write_RAWSXP(ctx, x_);
    break;
  case VECSXP:
    write_VECSXP(ctx, x_);
    break;
  case STRSXP:
    write_STRSXP(ctx, x_);
    break;  
  case LISTSXP: // pairlist
    write_LISTSXP(ctx, x_);
    break;
  case SYMSXP:
    write_SYMSXP(ctx, x_);
    break;
  case LANGSXP:
    write_LANGSXP(ctx, x_);
    break;
  case EXPRSXP:
    write_EXPRSXP(ctx, x_);
    break;
  case ENVSXP:
    write_ENVSXP(ctx, x_);
    break;
  case CLOSXP:
    write_CLOSXP(ctx, x_);
    break;
  default:
    {
      // For any SEXP which is not handled above e.g. BCODESXP, just
      // use R's serialization.  Eventually it'd be good to handle all
      // SEXPs in zap
      used_rserialize = true;
      write_rserialize(ctx, x_);
      possibly_has_attrs = false;
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - NILSXP can't have attributes
  // - If the object was handled by R's serialization mechanism attributes
  //   were handled there already
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (possibly_has_attrs) {
    write_attrs(ctx, x_);
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Log the SEXP 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ctx->opts->verbosity & ZAP_VERBOSITY_OBJDF) {
    
    SEXP objdf_ = VECTOR_ELT(ctx->cache, ZAP_CACHE_TALLY);
    if (ctx->obj_count >= ctx->obj_capacity) {
      df_grow(objdf_);
      ctx->obj_capacity *= 2;
    }
    
    
    int end = (int)get_position(ctx->user_data);
    objdf_add_row(objdf_, obj_count, ctx->depth, TYPEOF(x_), start,
                  end, ALTREP(x_), used_rserialize, possibly_has_attrs);
    
    ctx->depth--;
  }
  
  
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_sexp(ctx_t *ctx) {
  
  uint8_t type = (uint8_t)read_uint8(ctx);
  
  SEXP obj_ = R_NilValue;
  bool possibly_has_attrs = true;
  
  // Rprintf("Read type: %i\n", type);
  
  // Lower 5 bits *ALWAYS* holds the true SEXP type
  // The upper 3 bits may be repurposed to indicate 
  // special handling for this particular object without having
  // to insert an extra control byte in the stream.
  // E.g. see ENVSXP handling for repeated references
  switch(type & 0x1f) {  
  case NILSXP:
    obj_ = PROTECT(R_NilValue);
    possibly_has_attrs = false;
    break;
  case LGLSXP:
    obj_ = PROTECT(read_LGLSXP(ctx));
    break;
  case (REALSXP):
    obj_ = PROTECT(read_REALSXP(ctx, false));
    break;
  case INTSXP:
    obj_ = PROTECT(read_INTSXP(ctx));
    break;
  case FACTORSXP:
    obj_ = PROTECT(read_factor(ctx));
    break;
  case CPLXSXP:
    obj_ = PROTECT(read_REALSXP(ctx, true));
    break;
  case RAWSXP:
    obj_ = PROTECT(read_RAWSXP(ctx));
    break;
  case STRSXP:
    obj_ = PROTECT(read_STRSXP(ctx));
    break;
  case VECSXP:
    obj_ = PROTECT(read_VECSXP(ctx, type));
    break;
  case LISTSXP: // pairlist
    obj_ = PROTECT(read_LISTSXP(ctx));
    break;
  case SYMSXP:
    obj_ = PROTECT(read_SYMSXP(ctx));
    break;
  case LANGSXP:
    obj_ = PROTECT(read_LANGSXP(ctx));
    break;
  case EXPRSXP:
    obj_ = PROTECT(read_EXPRSXP(ctx));
    break;
  case ENVSXP:
    obj_ = PROTECT(read_ENVSXP(ctx));
    break;
  case CLOSXP:
    obj_ = PROTECT(read_CLOSXP(ctx));
    break;
  case RSERIALSXP:
    obj_ = PROTECT(read_rserialize(ctx));
    possibly_has_attrs = false;
    break;
  default:
    Rf_error("read_sexp() type not understood: %i %s", type, Rf_type2char(type));
  }
  
  if (possibly_has_attrs ) {
    read_attrs(ctx, obj_);
  }
  
  UNPROTECT(1);
  return obj_;
}



