

#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <zlib.h>

#include "io-ctx.h"
#include "io-STRSXP.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###                       
//  #   #                      
//  #       ###   # ##    ###  
//  #      #   #  ##  #  #   # 
//  #      #   #  #      ##### 
//  #   #  #   #  #      #     
//   ###    ###   #       ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_STRSXP(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // for 'short' STRXP (below the length threshold), just encode as 
  // raw lengths and character data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_length(x_) < ctx->opts->str_threshold) {
    write_STRSXP_raw(ctx, x_);
    return;
  }
  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Otherwise, use the user selected method for string encoding
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  switch(ctx->opts->str_transform) {
  case ZAP_STR_RAW:
    write_STRSXP_raw(ctx, x_);
    break;
  case ZAP_STR_MEGA:
    write_STRSXP_mega(ctx, x_);
    break;
  case ZAP_STR_DICT:
    write_STRSXP_dict(ctx, x_);
    break;
  default:
    Rf_error("write_STRSXP() str transform unknown %i", ctx->opts->str_transform);
  } 
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read STRSXP
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_STRSXP(ctx_t *ctx) {
  
  int method = read_uint8(ctx);
  
  switch(method) {
  case ZAP_STR_RAW:
    return read_STRSXP_raw(ctx);
    break;
  case ZAP_STR_MEGA:
    return read_STRSXP_mega(ctx);
    break;
  case ZAP_STR_DICT:
    return read_STRSXP_dict(ctx);
    break;
  default:
    Rf_error("read_STRSXP() str transform unknown %i", method);
  }
}




