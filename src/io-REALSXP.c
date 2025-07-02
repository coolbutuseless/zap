
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

#include "io-ctx.h"
#include "io-REALSXP.h"
#include "utils-shuffle.h"
#include "utils-ints.h"
#include "utils-alp.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Doubles are tricky to compress well. A cascade of methods is tried:
//
// - If length < 10, just write the RAW uncompressed values
// - Then probe to see if ALP is a good match
//     - If YES:  Encode to ALP
//     - if NO: encode via Delta+SHuffle
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ####                
// #   #               
// #   #   ###   #   # 
// ####       #  #   # 
// # #     ####  # # # 
// #  #   #   #  # # # 
// #   #   ####   # #            
// 
// Read/write raw untransformed values
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_REALSXP_raw(ctx_t *ctx, SEXP x_, bool is_complex) {
  
  // Header: SEXPTYPE + Encoding type
  write_uint8(ctx, is_complex ? CPLXSXP : REALSXP);
  write_uint8(ctx, ZAP_DBL_RAW);
  
  // 'len' = Num Doubles
  size_t len = (size_t)Rf_xlength(x_);
  if (is_complex) {
    len *= 2;
  }
  
  // Write length
  write_len(ctx, (uint64_t)len);
  if (len == 0) return;
  
  // Write data
  write_ptr(ctx, (void *)DATAPTR_RO(x_), len * sizeof(double));
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read uncompressed stream of doubles
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_REALSXP_raw(ctx_t *ctx, bool is_complex) {
  
  // Read length and create vector of correct type
  size_t len = (size_t)read_len(ctx);
  SEXP x_;
  if (is_complex) {
    x_ = PROTECT(Rf_allocVector(CPLXSXP, (R_xlen_t)len / 2)); 
  } else {
    x_ = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t)len)); 
  }
  
  // Early exit
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  // Read data directly into R object
  if (is_complex) {
    read_ptr(ctx, COMPLEX(x_));
  } else {
    read_ptr(ctx, REAL(x_));
  }

  
  UNPROTECT(1);
  return x_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###   #               ##     ##    ##          
//  #   #  #              #  #   #  #    #          
//  #      # ##   #   #   #      #       #     ###  
//   ###   ##  #  #   #  ####   ####     #    #   # 
//      #  #   #  #   #   #      #       #    ##### 
//  #   #  #   #  #  ##   #      #       #    #     
//   ###   #   #   ## #   #      #      ###    ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUF_SHUFFLE    0

void write_REALSXP_shuffle(ctx_t *ctx, SEXP x_, bool is_complex) {
  
  // Write: sexptype + encoding type + length
  write_uint8(ctx, is_complex ? CPLXSXP : REALSXP);
  write_uint8(ctx, ZAP_DBL_SHUF);
  
  // 'len' is number of doubles
  size_t len = (size_t)Rf_xlength(x_);
  if (is_complex) {
    len *= 2;
  }
  
  // write the length
  write_len(ctx, (uint64_t)len);
  
  // early return
  if (len == 0) return;
  
  // Delta+Shuffle the 8-bytes in each double
  shuffle8_ptr_buf(ctx, (void *)DATAPTR_RO(x_), BUF_SHUFFLE, len);
  
  // Write the compressed data
  write_buf(ctx, BUF_SHUFFLE, len * sizeof(double));
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read Delta+Shuffled data
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_REALSXP_shuffle(ctx_t *ctx, bool is_complex) {
  
  // Read the length and create an empty vector of the correct type
  size_t len = (size_t)read_len(ctx);
  SEXP x_;
  if (is_complex) {
    x_ = PROTECT(Rf_allocVector(CPLXSXP, (R_xlen_t)len / 2)); 
  } else {
    x_ = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t)len)); 
  }
  
  // Early exit
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  // Read compressed data and decompress
  read_buf(ctx, BUF_SHUFFLE);
  
  // Unshuffle
  if (is_complex) {
    unshuffle8_buf_ptr(ctx, BUF_SHUFFLE, COMPLEX(x_), len);
  } else {
    unshuffle8_buf_ptr(ctx, BUF_SHUFFLE, REAL(x_), len);
  }
  
  
  UNPROTECT(1);
  return x_;
}

#undef BUF_SHUFFLE



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ####           ##     #             ###   #               ##  
//   #  #           #     #            #   #  #              #  # 
//   #  #   ###     #    ####    ###   #      # ##   #   #   #    
//   #  #  #   #    #     #         #   ###   ##  #  #   #  ####  
//   #  #  #####    #     #      ####      #  #   #  #   #   #    
//   #  #  #        #     #  #  #   #  #   #  #   #  #  ##   #    
//  ####    ###    ###     ##    ####   ###   #   #   ## #   #    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUF_SHUFFLE    0

void write_REALSXP_delta_shuffle(ctx_t *ctx, SEXP x_, bool is_complex) {

  // Write: sexptype + encoding type + length
  write_uint8(ctx, is_complex ? CPLXSXP : REALSXP);
  write_uint8(ctx, ZAP_DBL_SHUF_DELTA);
  
  // 'len' is number of doubles
  size_t len = (size_t)Rf_xlength(x_);
  if (is_complex) {
    len *= 2;
  }
  
  // write the length
  write_len(ctx, (uint64_t)len);
  
  // early return
  if (len == 0) return;
  
  // Delta+Shuffle the 8-bytes in each double
  shuffle_delta8_ptr_buf(ctx, (void *)DATAPTR_RO(x_), BUF_SHUFFLE, len);
  
  // Write the compressed data
  write_buf(ctx, BUF_SHUFFLE, len * sizeof(double));
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read Delta+Shuffled data
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_REALSXP_delta_shuffle(ctx_t *ctx, bool is_complex) {
  
  // Read the length and create an empty vector of the correct type
  size_t len = (size_t)read_len(ctx);
  SEXP x_;
  if (is_complex) {
    x_ = PROTECT(Rf_allocVector(CPLXSXP, (R_xlen_t)len / 2)); 
  } else {
    x_ = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t)len)); 
  }
  
  // Early exit
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  // Read compressed data and decompress
  read_buf(ctx, BUF_SHUFFLE);
  
  // Unshuffle
  if (is_complex) {
    unshuffle_delta8_buf_ptr(ctx, BUF_SHUFFLE, COMPLEX(x_), len);
  } else {
    unshuffle_delta8_buf_ptr(ctx, BUF_SHUFFLE, REAL(x_), len);
  }
  
  
  UNPROTECT(1);
  return x_;
}

#undef BUF_SHUFFLE







#define BUF_ALP       0
#define BUF_PATCH     1
#define BUF_PATCH_IDX 2
#define BUF_SHUF      3
#define BUF_COMP      4

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//     #    #      ####  
//    # #   #      #   # 
//   #   #  #      #   # 
//   #   #  #      ####  
//   #####  #      #     
//   #   #  #      #     
//   #   #  #####  #     
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_REALSXP_alp0(ctx_t *ctx, SEXP x_, bool is_complex) {
  
  size_t len = (size_t)Rf_xlength(x_);
  if (is_complex) {
    len *= 2;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get an appropriate pointer to the data.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double *x = is_complex ? (double *)COMPLEX(x_) : REAL(x_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ALP probe
  // We will probe for 'Nsample' equi-spaced values along the vector.
  // If length of input < 'Nsample' then just check every element.
  //
  // The results of the probe (the 'e' and 'f' indices) are part of the
  // pparams struct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t Nsample = 256;
  alp_params_t pparams = alp_probe(x, len, Nsample);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Short circuit - if it doesn't look like this will compress well with ALP,
  // then just use the simpler method.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (((double)pparams.score / (double)pparams.ntest ) < 0.9) {
    switch (ctx->opts->dbl_fallback) {
    case ZAP_DBL_RAW:
      write_REALSXP_raw(ctx, x_, is_complex);
      break;
    case ZAP_DBL_SHUF:
      write_REALSXP_shuffle(ctx, x_, is_complex);
      break;
    case ZAP_DBL_SHUF_DELTA:
      write_REALSXP_delta_shuffle(ctx, x_, is_complex);
      break;
    default:
      Rf_error("REALSXP: unknown fallback");
    }
    return;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write the header for the ALP data
  //  - sexptype
  //  - encoding type
  //  - length
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_uint8(ctx, is_complex ? CPLXSXP : REALSXP);
  write_uint8(ctx, ZAP_DBL_ALP);
  write_len(ctx, (uint64_t)len);
  
  if (len == 0) return;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Prepare buffers to hold ALP results
  //   BUF_ALP   = the encoded int64_t values
  //   PATCH     = the values at the patch locations (where ALP wasn't effective)
  //   PATCH_IDX = the index of the path locations 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  prepare_buf(ctx, BUF_ALP      , len * sizeof(double));
  prepare_buf(ctx, BUF_PATCH    , len * sizeof(double));
  prepare_buf(ctx, BUF_PATCH_IDX, len * sizeof(uint32_t));
  
  // How many patches were required?
  uint32_t npatch = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Encode the doubles into int64_t
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  alp_encode(
    x,                                   // src of doubles
    (int64_t *)ctx->buf[BUF_ALP],        // encoded int64_t
    len,                                 // number of doubles
    pparams.e,                           // best e param
    pparams.f,                           // best f param
    (double *)ctx->buf[BUF_PATCH],       // storage for patch values
    (uint32_t *)ctx->buf[BUF_PATCH_IDX], // storage for patch locations
    &npatch                              // number of patches
  );
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Debugging: Have a look at the patches
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (0) {
    Rprintf("Npatch: %i\n", npatch);
    double *patch = (double *)ctx->buf[BUF_PATCH];
    int *patch_idx = (int *)ctx->buf[BUF_PATCH_IDX];
    for (int i = 0; i < npatch; i++) {
      Rprintf("[%4i] [%4i] %f\n", i, patch_idx[i], patch[i]);
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write patches.
  // BUF_PATCH_IDX is an unsigned integer of positions
  // BUF_PATCH is an array of doubles.  Probably quite repetitive with NA values
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_uint32_buf(ctx, BUF_PATCH_IDX, npatch); // npatch = n_ints
  write_buf  (ctx, BUF_PATCH    , npatch * sizeof(double));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Delta+Shuffle and compress the ALP encoding
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  shuffle_delta8_buf_buf(ctx, BUF_ALP, BUF_SHUF, len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write the ALP 'e' and 'f' paremeters
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_uint8(ctx, (uint8_t)pparams.e);
  write_uint8(ctx, (uint8_t)pparams.f);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write the compressed data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_buf(ctx, BUF_SHUF, len * sizeof(double));
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read ALP compressed doubles
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_REALSXP_alp0(ctx_t *ctx, bool is_complex) {
  
  size_t len = (size_t)read_len(ctx);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise an object of the correct type
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP x_;
  if (is_complex) {
    x_ = PROTECT(Rf_allocVector(CPLXSXP, (R_xlen_t)len / 2)); 
  } else {
    x_ = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t)len)); 
  }
  
  // Early return
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read patches
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t npatch = read_uint32_buf(ctx, BUF_PATCH_IDX);
  read_buf(ctx, BUF_PATCH);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read key parameters for ALP: 'e' and 'f'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint8_t e = read_uint8(ctx);
  uint8_t f = read_uint8(ctx);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Uncompress the ALP data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_SHUF);
  unshuffle_delta8_buf_buf(ctx, BUF_SHUF, BUF_ALP, len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ALP decode
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double *x = is_complex ? (double *)COMPLEX(x_) : REAL(x_);
  
  alp_decode(
    (int64_t *)ctx->buf[BUF_ALP],     // ALP encoded data
    x,                                // destination for doubles
    len,                              // number of doubles
    e,                                // ALP parameter
    f,                                // ALP parameter
    (void *)ctx->buf[BUF_PATCH],      // Patch values
    (void *)ctx->buf[BUF_PATCH_IDX],  // Patch indices
    (uint32_t)npatch                  // number of patches
  );
  
  UNPROTECT(1);
  return x_;
}




void write_REALSXP(ctx_t *ctx, SEXP x_, bool is_complex) {
  
  if (Rf_length(x_) < ctx->opts->dbl_threshold) {
    write_REALSXP_raw(ctx, x_, is_complex);
    return;
  }
  
  
  switch(ctx->opts->dbl_transform) {
  case ZAP_DBL_RAW:
    write_REALSXP_raw(ctx, x_, is_complex);
    break;
  case ZAP_DBL_SHUF:
    write_REALSXP_shuffle(ctx, x_, is_complex);
    break;
  case ZAP_DBL_SHUF_DELTA:
    write_REALSXP_delta_shuffle(ctx, x_, is_complex);
    break;
  case ZAP_DBL_ALP:
    write_REALSXP_alp0(ctx, x_, is_complex);
    break;
  default:
    Rf_error("write_REALSXP(): dbl transform not known: %i", ctx->opts->dbl_transform);
  }
  
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The READ function dispatches on the 'encoding method' byte
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_REALSXP(ctx_t *ctx, bool is_complex) {
  
  uint8_t method = read_uint8(ctx);
  
  switch(method) {
  case ZAP_DBL_RAW:
    return read_REALSXP_raw(ctx, is_complex);
    break;
  case ZAP_DBL_SHUF:
    return read_REALSXP_shuffle(ctx, is_complex);
    break;
  case ZAP_DBL_SHUF_DELTA:
    return read_REALSXP_delta_shuffle(ctx, is_complex);
    break;
  case ZAP_DBL_ALP:
    return read_REALSXP_alp0(ctx, is_complex);
    break;
  default:
    Rf_error("read_REALSXP(): method not understood: %i", method);
  }
}





