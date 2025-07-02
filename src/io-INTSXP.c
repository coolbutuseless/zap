
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

#include "io-INTSXP.h"
#include "utils-zigzag.h"
#include "utils-shuffle.h"
#include "utils-int-frame-delta.h"
#include "utils-packing-1bit.h"


#define BUF_ZIGZAG     0
#define BUF_FRAME      0
#define BUF_NA_PACKED  1
#define BUF_SHUFFLE    2


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ####                
//   #   #               
//   #   #   ###   #   # 
//   ####       #  #   # 
//   # #     ####  # # # 
//   #  #   #   #  # # # 
//   #   #   ####   # #  
//
//  Just writing out the integers "as-is" with no processing or transformation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_INTSXP_raw(ctx_t *ctx, SEXP x_) {
  write_uint8(ctx, INTSXP);      // SEXP
  write_uint8(ctx, ZAP_INT_RAW); // Integer encoding type
  
  size_t len = (size_t)Rf_xlength(x_);
  write_len(ctx, len);
  if (len == 0) return;
  
  ctx->write(ctx->user_data, (void *)DATAPTR_RO(x_), len * sizeof(int32_t));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read uncompressed integer data
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_INTSXP_raw(ctx_t *ctx) {
  
  size_t len = (size_t)read_len(ctx);
  SEXP x_ = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t)len)); 
  
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }

  ctx->read(ctx->user_data, INTEGER(x_), len * sizeof(int32_t));
    
  UNPROTECT(1);
  return x_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  #####  #####          ###   #               ##     ##    ##          
//      #      #         #   #  #              #  #   #  #    #          
//     #      #          #      # ##   #   #   #      #       #     ###  
//    #      #            ###   ##  #  #   #  ####   ####     #    #   # 
//   #      #                #  #   #  #   #   #      #       #    ##### 
//  #      #             #   #  #   #  #  ##   #      #       #    #     
//  #####  #####          ###   #   #   ## #   #      #      ###    ###  
//
// ZigZag encode the integers - this encodes a signed int32 in a uint32 in 
//        a such a way that the "sign bit" isn't a determent to compression
// Shufffle - shuffle bytes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_INTSXP_zzshuf(ctx_t *ctx, SEXP x_) {
  
  write_uint8(ctx, INTSXP);           // SEXP
  write_uint8(ctx, ZAP_INT_ZZSHUF);  // Integer encoding type
  
  size_t len = (size_t)Rf_xlength(x_);
  write_len(ctx, (uint64_t)len);
  
  if (len == 0) return;
  
  zigzag_encode_ptr_buf(ctx, INTEGER(x_), BUF_ZIGZAG, len);
  shuffle_delta4_buf_buf(ctx, BUF_ZIGZAG, BUF_SHUFFLE, len);
  write_buf(ctx, BUF_SHUFFLE, len * sizeof(int));
}


SEXP read_INTSXP_zzshuf(ctx_t *ctx) {
  
  size_t len = (size_t)read_len(ctx);
  SEXP x_ = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t)len)); 
  
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  read_buf(ctx, BUF_SHUFFLE);
  unshuffle_delta4_buf_buf(ctx, BUF_SHUFFLE, BUF_ZIGZAG, len);
  zigzag_decode_buf_ptr(ctx, BUF_ZIGZAG, INTEGER(x_), len);
  
  
  UNPROTECT(1);
  return x_;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ####           ##     #                   #####   ###   ####  
//   #  #           #     #                   #      #   #  #   # 
//   #  #   ###     #    ####    ###          #      #   #  #   # 
//   #  #  #   #    #     #         #         ####   #   #  ####  
//   #  #  #####    #     #      ####         #      #   #  # #   
//   #  #  #        #     #  #  #   #         #      #   #  #  #  
//  ####    ###    ###     ##    ####         #       ###   #   # 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_INTSXP_deltaframe(ctx_t *ctx, SEXP x_) {
  
  
  size_t len = (size_t)Rf_xlength(x_);
  
  
  int32_t ref = 0;  // reference level for encoding. 
  int32_t delta_offset = 0;
  size_t nbits = 0;
  size_t nbytes = deltaframe_encode_ptr_buf(ctx, INTEGER(x_), BUF_FRAME, len, &ref, &delta_offset, &nbits);
  
  if (nbits == 32) {
    // There are too many bits required to encode the 'delta' between elements
    // which makes this integer encoding mode impractical.  So fall-back to 
    // just using ZZShuf
    write_INTSXP_zzshuf(ctx, x_);
    return;
  } 
  
  write_uint8(ctx, INTSXP);              // SEXP 
  write_uint8(ctx, ZAP_INT_DELTAFRAME);  // Integer encoding type
  write_len(ctx, (uint64_t)len);
  if (len == 0) return;
  
  
  write_len(ctx, nbits);
  write_int32(ctx, ref);
  write_int32(ctx, delta_offset);
  write_buf(ctx, BUF_FRAME, nbytes);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the auxilliary bitstream of NA locations
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t packed_len = pack_na_int(ctx, BUF_NA_PACKED, x_);
  write_buf(ctx, BUF_NA_PACKED, packed_len);
}


SEXP read_INTSXP_deltaframe(ctx_t *ctx) {
  
  size_t len = (size_t)read_len(ctx);
  SEXP x_ = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t)len)); 
  
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  size_t nbits  = read_len(ctx);
  
  if (nbits == 32) {
    Rf_error("read_INTSXP_deltaframe() nbits == 32"); 
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the delta-frame-of-reference encoded integers 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int32_t ref   = read_int32(ctx);
  int32_t delta_offset = read_int32(ctx);
  read_buf(ctx, BUF_FRAME);
  deltaframe_decode_buf_ptr(ctx, BUF_FRAME, INTEGER(x_), len, ref, delta_offset, nbits);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set NA values using the auxilliary NA bistream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_NA_PACKED);
  unpack_na_int(ctx, BUF_NA_PACKED, x_, len);

  
  UNPROTECT(1);
  return x_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###                       
//  #   #                      
//  #       ###   # ##    ###  
//  #      #   #  ##  #  #   # 
//  #      #   #  #      ##### 
//  #   #  #   #  #      #     
//   ###    ###   #       ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_INTSXP(ctx_t *ctx, SEXP x_) {
  
  // When vector length is below the threshold, just write 
  // the raw bytes without transformation
  if (Rf_length(x_) < ctx->opts->int_threshold) {
    write_INTSXP_raw(ctx, x_);
    return;
  }
  
  
  switch(ctx->opts->int_transform) {
  case ZAP_INT_RAW:
    write_INTSXP_raw(ctx, x_);
    break;
  case ZAP_INT_ZZSHUF:
    write_INTSXP_zzshuf(ctx, x_);
    break;
  case ZAP_INT_DELTAFRAME:
    write_INTSXP_deltaframe(ctx, x_);
    break;
  default:
    Rf_error("write_INTSXP(): method unknown %i", ctx->opts->int_transform);
  }
  
}

SEXP read_INTSXP(ctx_t *ctx) {
  int method = read_uint8(ctx);
  
  switch(method) {
  case ZAP_INT_RAW:
    return read_INTSXP_raw(ctx);
    break;
  case ZAP_INT_ZZSHUF:
    return read_INTSXP_zzshuf(ctx);
    break;
  case ZAP_INT_DELTAFRAME:
    return read_INTSXP_deltaframe(ctx);
    break;
  default:
    Rf_error("read_INTSXP(): method unknown %i", method);
  }
}


