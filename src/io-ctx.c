


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


//===========================================================================
// Parse the R list of options into the 'opts_t' options struct
//
// @param opts_ An R named list of options. Passed in from the user.
//===========================================================================
opts_t *parse_options(SEXP opts_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate 'opts_t *'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  opts_t *opts = calloc(1, sizeof(opts_t));
  if (opts == NULL) {
    Rf_error("parse_options(): Could not allocate 'opts_t'");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set default options
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  opts->verbosity      = 0;
  opts->lgl_transform  = ZAP_LGL_PACKED;
  opts->int_transform  = ZAP_INT_DELTAFRAME;
  opts->fct_transform  = ZAP_FCT_PACKED;
  opts->dbl_transform  = ZAP_DBL_ALP;
  opts->str_transform  = ZAP_STR_MEGA;
  
  opts->dbl_fallback   = ZAP_DBL_SHUF_DELTA;
  
  opts->lgl_threshold =  0;
  opts->int_threshold =  0;
  opts->fct_threshold =  0;
  opts->dbl_threshold =  0;
  opts->str_threshold =  0;
  
  
  // Sanity check and extract option names from the named list
  if (Rf_isNull(opts_) || Rf_length(opts_) == 0) {
    return opts;
  }
  
  if (!Rf_isNewList(opts_)) {
    Rf_error("'opts' must be a list");
  }
  
  SEXP nms_ = Rf_getAttrib(opts_, R_NamesSymbol);
  if (Rf_isNull(nms_)) {
    Rf_error("'opts' must be a named list");
  }
  
  // Loop over options in R named list and assign to C struct
  for (int i = 0; i < Rf_length(opts_); i++) {
    const char *opt_name = CHAR(STRING_ELT(nms_, i));
    SEXP val_ = VECTOR_ELT(opts_, i);
    
    if (strcmp(opt_name, "verbosity") == 0) {
      opts->verbosity = Rf_asInteger(val_);
    } 
    
    else if (strcmp(opt_name, "transform") == 0) {
      if (!Rf_asLogical(val_)) {
        opts->lgl_transform = 0;
        opts->int_transform = 0;
        opts->fct_transform = 0;
        opts->dbl_transform = 0;
        opts->str_transform = 0;
      }
      
    } else if (strcmp(opt_name, "lgl") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      if (strcmp(val, "raw") == 0) {
        opts->lgl_transform = ZAP_LGL_RAW;
      } else if (strcmp(val, "packed") == 0) {
        opts->lgl_transform = ZAP_LGL_PACKED;
      } else {
        Rf_warning("Option not understood: lgl = '%s'. Using 'packed'", val);
        opts->lgl_transform = ZAP_LGL_PACKED;
      }
      
    } else if (strcmp(opt_name, "int") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      if (strcmp(val, "raw") == 0) {
        opts->int_transform = ZAP_INT_RAW;
      } else if (strcmp(val, "zzshuf") == 0) {
        opts->int_transform = ZAP_INT_ZZSHUF;
      } else if (strcmp(val, "deltaframe") == 0) {
        opts->int_transform = ZAP_INT_DELTAFRAME;
      } else {
        Rf_warning("Option not understood: int = '%s'. Using 'deltaframe'", val);
        opts->int_transform = ZAP_INT_DELTAFRAME;
      }
      
    } else if (strcmp(opt_name, "fct") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      if (strcmp(val, "raw") == 0) {
        opts->fct_transform = ZAP_FCT_RAW;
      } else  if (strcmp(val, "packed") == 0) {
        opts->fct_transform = ZAP_FCT_PACKED;
      } else {
        Rf_warning("Option not understood: fct = '%s'. Using 'packed'", val);
        opts->fct_transform = ZAP_FCT_PACKED;
      }
      
    } else if (strcmp(opt_name, "dbl") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      if (strcmp(val, "raw") == 0) {
        opts->dbl_transform = ZAP_DBL_RAW;
      } else if (strcmp(val, "shuffle") == 0) {
        opts->dbl_transform = ZAP_DBL_SHUF;
      } else if (strcmp(val, "delta_shuffle") == 0) {
        opts->dbl_transform = ZAP_DBL_SHUF_DELTA;
      } else if (strcmp(val, "alp") == 0) {
        opts->dbl_transform = ZAP_DBL_ALP;
      } else {
        Rf_warning("Option not understood: dbl = '%s'. Using 'alp'", val);
        opts->dbl_transform = ZAP_DBL_ALP;
      }
      
    } else if (strcmp(opt_name, "dbl_fallback") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      if (strcmp(val, "raw") == 0) {
        opts->dbl_fallback = ZAP_DBL_RAW;
      } else if (strcmp(val, "shuffle") == 0) {
        opts->dbl_fallback = ZAP_DBL_SHUF;
      } else if (strcmp(val, "delta_shuffle") == 0) {
        opts->dbl_fallback = ZAP_DBL_SHUF_DELTA;
      } else {
        Rf_warning("Option not understood: dbl_fallback = '%s'. Using 'delta_shuffle'", val);
        opts->dbl_fallback = ZAP_DBL_SHUF_DELTA;
      }
      
    } else if (strcmp(opt_name, "str") == 0) {
      const char *val = CHAR(STRING_ELT(val_, 0));
      if (strcmp(val, "raw") == 0) {
        opts->str_transform = ZAP_STR_RAW;
      } else if (strcmp(val, "mega") == 0) {
        opts->str_transform = ZAP_STR_MEGA;
      } else {
        Rf_warning("Option not understood: str = '%s'. Using 'mega'", val);
        opts->str_transform = ZAP_STR_MEGA;
      }
      
    } else if (strcmp(opt_name, "lgl_threshold") == 0) {
      opts->lgl_threshold = Rf_asInteger(val_); 
    } else if (strcmp(opt_name, "int_threshold") == 0) {
      opts->int_threshold = Rf_asInteger(val_); 
    } else if (strcmp(opt_name, "fct_threshold") == 0) {
      opts->fct_threshold = Rf_asInteger(val_); 
    } else if (strcmp(opt_name, "dbl_threshold") == 0) {
      opts->dbl_threshold = Rf_asInteger(val_); 
    } else if (strcmp(opt_name, "str_threshold") == 0) {
      opts->str_threshold = Rf_asInteger(val_); 

    } else {
      Rf_warning("Unknown option ignored: '%s'\n", opt_name);
    }
  }
  
  return opts;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Expand a buffer - ignoring the current contents
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void prepare_buf(ctx_t *ctx, int idx, size_t len) {
  if (idx < 0 || idx >= CTX_NBUFS) Rf_error("buf idx out-of-range: %i", idx);
  
  if (len <= ctx->bufsize[idx]) {
    return;
  }
  
  ctx->bufsize[idx] = len;
  free(ctx->buf[idx]);
  ctx->buf[idx]  = malloc(ctx->bufsize[idx]);
  
  if (ctx->buf[idx] == NULL) {
    Rf_error("prepare_buf(): Couldn't prepare ctx->buf[%i] with %li", idx, (long)len);
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Re-allocate a buffer - preserving the contents
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void realloc_buf(ctx_t *ctx, int idx, size_t len) {
  if (idx < 0 || idx >= CTX_NBUFS) Rf_error("buf idx out-of-range: %i", idx);
  
  if (len <= ctx->bufsize[idx]) {
    return;
  }
  
  ctx->bufsize[idx] = len;
  ctx->buf[idx] = realloc(ctx->buf[idx], ctx->bufsize[idx]);
  
  if (ctx->buf[idx] == NULL) {
    Rf_error("realloc_buf(): Couldn't re-allocate ctx->buf");
  }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read/Write from buffer
//
// When reading, buffer will be resized to hold [LEN] bytes
//
// Format: [length] [DATA]
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_buf(ctx_t *ctx, int buf_idx, size_t len) {
  write_len(ctx, len);
  
  if (len > 0) {
    ctx->write(ctx->user_data, ctx->buf[buf_idx], len);
  }
}

size_t read_buf(ctx_t *ctx, int buf_idx) {
  
  size_t len = read_len(ctx);
  if (len == 0) return 0;
  
  // Ensure there's enough room in the buffer to read the data
  prepare_buf(ctx, buf_idx, len);
  ctx->read(ctx->user_data, ctx->buf[buf_idx], len);
  
  return len;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read/Write from ptr 
//
// Caller must ensure that memory has been allocated 
//
// Format: [length] [DATA]
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_ptr(ctx_t *ctx, void *ptr, size_t len) {
  write_len(ctx, len);
  
  if (len > 0) {
    ctx->write(ctx->user_data, ptr, len);
  }
}

size_t read_ptr(ctx_t *ctx, void *ptr) {
  
  size_t len = read_len(ctx);
  if (len == 0) return 0;
  
  ctx->read(ctx->user_data, ptr, len);
  
  return len;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read/Write a single signed int32
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_int32(ctx_t *ctx, int32_t val) {
  ctx->write(ctx->user_data, &val, sizeof(int32_t));
}

int32_t read_int32(ctx_t *ctx) {
  int32_t val = 0;  
  ctx->read(ctx->user_data, &val, sizeof(int32_t));
  return val;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read/Write a single byte
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_uint8(ctx_t *ctx, uint8_t val) {
  ctx->write(ctx->user_data, &val, sizeof(uint8_t));
}

uint8_t read_uint8(ctx_t *ctx) {
  uint8_t val = 0;  
  ctx->read(ctx->user_data, &val, sizeof(uint8_t));
  return val;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Buffers are initially sized at 128k each, but can grow
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define INIT_BUFSIZE 128 * 1024


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create a context for reading/writing R objects
// @param loc_ filename.  But in future could also be R connetion object, 
//        of maybe "NULL" to indicate
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ctx_t *create_ctx(opts_t *opts) {
  ctx_t *ctx = calloc(1, sizeof(ctx_t));
  
  // Parse any extra options from the user
  ctx->opts = opts;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // allocate scratch buffers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < CTX_NBUFS; i++) {
    ctx->bufsize[i] = INIT_BUFSIZE;
    ctx->buf[i]     = malloc(ctx->bufsize[i]);
    if (ctx->buf[i] == NULL) {
      Rf_error("create_ctx(): Couldn't allocate ctx->buf");
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // allocate space to store caches
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ctx->cache = PROTECT(Rf_allocVector(VECSXP, 2));
  R_PreserveObject(ctx->cache);
  UNPROTECT(1);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup a cache of environment
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ctx->Nenv = 0;
  SEXP env_cache_ = PROTECT(Rf_allocVector(VECSXP, 4));
  SET_VECTOR_ELT(ctx->cache, ZAP_CACHE_ENVSXP, env_cache_);
  UNPROTECT(1);
  
  
  return ctx;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create a context for serializing 
// - used internally
// - also part of the external C API.  See init.c and inst/include/zap.h
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ctx_t *create_serialize_ctx(void *user_data, 
                            void    (*write)(void *user_data, void *buf, size_t len),
                            opts_t *opts) {
  ctx_t *ctx     = create_ctx(opts);
  ctx->user_data = user_data;
  
  ctx->write = write;
  
  return ctx;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create a context for serializing 
// - used internally
// - also part of the external C API.  See init.c and inst/include/zap.h
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ctx_t *create_unserialize_ctx(void *user_data,
                              void    (*read)(void *user_data, void *buf, size_t len),
                              opts_t *opts) {
  ctx_t *ctx     = create_ctx(opts);
  ctx->user_data = user_data;
  
  ctx->read  = read;
  
  return ctx;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Destroy a context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ctx_destroy(ctx_t *ctx) {
  if (ctx == NULL) return;
  
  if ((ctx->opts != NULL) && (ctx->opts->verbosity & ZAP_VERB_TALLY)) {
    dump_tally(ctx);
  }
  
  for (int i = 0; i < CTX_NBUFS; i++) {
    free(ctx->buf[i]);
  }
  
  R_ReleaseObject(ctx->cache); // R object cache
  free(ctx);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Character strings used for verbose output
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
char *sexp_nms[32] = {
  "NILSXP"	    , //   0	NILSXP	    NULL 
  "SYMSXP"	    , //   1	SYMSXP	    symbols 
  "LISTSXP"	    , //   2	LISTSXP	    pairlists 
  "CLOSXP"	    , //   3	CLOSXP	    closures 
  "ENVSXP"	    , //   4	ENVSXP	    environments 
  "PROMSXP"	    , //   5	PROMSXP	    promises 
  "LANGSXP"	    , //   6	LANGSXP	    language objects 
  "SPECIALSXP"  , //   7	SPECIALSXP	special functions 
  "BUILTINSXP"  , //   8	BUILTINSXP	builtin functions 
  "CHARSXP"	    , //   9	CHARSXP	    internal character strings 
  "LGLSXP"	    , //  10	LGLSXP	    logical vectors 
  "unused"      , //  11  unused
  "unused"      , //  12  unused
  "INTSXP"	    , //  13	INTSXP	    integer vectors 
  "REALSXP"	    , //  14	REALSXP	    numeric vectors 
  "CPLXSXP"	    , //  15	CPLXSXP	    complex vectors 
  "STRSXP"	    , //  16	STRSXP	    character vectors 
  "DOTSXP"	    , //  17	DOTSXP	    dot-dot-dot object 
  "ANYSXP"	    , //  18	ANYSXP	    make “any” args work 
  "VECSXP"	    , //  19	VECSXP	    list (generic vector) 
  "EXPRSXP"	    , //  20	EXPRSXP	    expression vector 
  "BCODESXP"    , //  21	BCODESXP   	byte code 
  "EXTPTRSXP"   , //  22	EXTPTRSXP  	external pointer 
  "WEAKREFSXP" , //  23	WEAKREFSXP	weak reference 
  "RAWSXP"     , //  24	RAWSXP    	raw vector 
  "S4SXP"	     , //  25	S4SXP	      S4 classes not of simple type 
  "unused"     ,
  "unused"     ,
  "unused"     ,
  "unused"     ,
  "unused"     ,
  "unused"     
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Dump three tallies to screen
//  - all SEXPs
//  - ALTREPs
//  - SEXPs which were handled with R's serialization framework
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void dump_tally(ctx_t *ctx) {
  
  bool has_values = false;
  for (int i = 0; i < 32; i++) {
    has_values |= (ctx->tally_sexp[i] > 0);
  }
  if (has_values) {
    Rprintf("SEXP\n");
    for (int i = 0; i < 32; i++) {
      if (ctx->tally_sexp[i] > 0) {
        Rprintf("    %-13s %3i\n", sexp_nms[i], ctx->tally_sexp[i]);
      }
    }
  }
  
  has_values = false;
  for (int i = 0; i < 32; i++) {
    has_values |= (ctx->tally_altrep[i] > 0);
  }
  if (has_values) {
    Rprintf("ALTREP\n");
    for (int i = 0; i < 32; i++) {
      if (ctx->tally_altrep[i] > 0) {
        Rprintf("    %-13s %3i\n", sexp_nms[i], ctx->tally_altrep[i]);
      }
    }
  }
  
  has_values = false;
  for (int i = 0; i < 32; i++) {
    has_values |= (ctx->tally_serial[i] > 0);
  }
  if (has_values) {
    Rprintf("RSerialize\n");
    for (int i = 0; i < 32; i++) {
      if (ctx->tally_serial[i] > 0) {
        Rprintf("    %-13s %3i\n", sexp_nms[i], ctx->tally_serial[i]);
      }
    }
  }
}










