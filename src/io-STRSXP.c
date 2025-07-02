

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
#include "utils-packing-1bit.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ####                
//   #   #               
//   #   #   ###   #   # 
//   ####       #  #   # 
//   # #     ####  # # # 
//   #  #   #   #  # # # 
//   #   #   ####   # #  
//
// Read/write raw untransformed strings.
// i.e. Sequence of (strlen, string) objects
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUF_CHR 0

void write_STRSXP_raw(ctx_t *ctx, SEXP x_) {
  
  write_uint8(ctx, STRSXP);
  write_uint8(ctx, ZAP_STR_RAW);
  
  R_xlen_t len = Rf_xlength(x_);
  write_len(ctx, (uint64_t)len);
  
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP chr_ = STRING_ELT(x_, i);
    int is_na = (chr_ == NA_STRING);
    write_uint8(ctx, (uint8_t)is_na);
    if (!is_na) {
      write_ptr(ctx, (void *)CHAR(chr_), (size_t)Rf_length(chr_));  // Include \0 terminator
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_STRSXP_raw(ctx_t *ctx) {
  
  R_xlen_t len = (R_xlen_t)read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(STRSXP, len)); 
  
  for (R_xlen_t i = 0; i < len; i++) {
    int is_na = read_uint8(ctx);
    if (is_na) {
      SET_STRING_ELT(obj_, i, NA_STRING);
    } else {
      size_t slen = read_buf(ctx, BUF_CHR);
      SET_STRING_ELT(obj_, i, Rf_mkCharLen((const char *)ctx->buf[BUF_CHR], (int)slen));
    }
  }
  
  UNPROTECT(1);
  return obj_;
}

#undef BUF_CHR





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    #   #                       
//    #   #                       
//    ## ##   ###    ## #   ###   
//    # # #  #   #  #  #       #  
//    #   #  #####   ##     ####  
//    #   #  #      #      #   #  
//    #   #   ###    ###    ####  
//                  #   #         
//                   ###          
//
// Concatenate all strings into a single mega string which uses the NUL
// byte as the delimiter between strings.
// This means we don't need to write the length for each string, just the
// overall mega-string
//
// Locations of 'NA' strings is stored separately in a custom bit-array where
// "is NA" is encoded as 1-bit-per-string
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define BUF_NA_PACKED    0
#define BUF_RAW          1
#define BUF_COMP         2

void write_STRSXP_mega(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write:
  //  * [1] SEXP
  //  * [v] Number of strings
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_uint8(ctx, STRSXP);
  write_uint8(ctx, ZAP_STR_MEGA);
  
  size_t len = (size_t)Rf_length(x_);
  write_len(ctx, len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check for empty vector and return early
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (len == 0) return;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find lengths of all strings. (Including NULL byte)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint64_t total_chars = 0;
  for (int i = 0; i < len; i++) {
    total_chars += ((uint64_t)Rf_length(STRING_ELT(x_, i)) + 1); // count zero bytes
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // length of mega should be represented by 8 bytes possibly?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (total_chars > (1ULL << 32)) Rf_error("write_STRSXP(): string too long");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Total Chars
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_len(ctx, total_chars);
  if (total_chars == len) return; // all empty strings
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the auxilliary bitstream of NA locations
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t packed_len = pack_na_str(ctx, BUF_NA_PACKED, x_);
  write_buf(ctx, BUF_NA_PACKED, packed_len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate storage space for the entire long string
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  prepare_buf(ctx, BUF_RAW, total_chars + 1);
  char *p = (char *)ctx->buf[BUF_RAW];
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the mega concatenated string
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < len; i++) {
    SEXP chr_ = STRING_ELT(x_, i);
    unsigned long slen = (unsigned long)Rf_length(chr_) + 1;
    strncpy(p, CHAR(chr_), slen);
    p += slen;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Output character data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_buf(ctx, BUF_RAW, (size_t)total_chars);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_STRSXP_mega(ctx_t *ctx) {
  
  size_t len = read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t)len)); 
  
  if (len == 0) {
    UNPROTECT(1);
    return obj_;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Character data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t total_chars = read_len(ctx);
  if (total_chars == len) {
    // Character vector of empty strings
    UNPROTECT(1);
    return obj_;
  }
  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // NA locations
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_NA_PACKED);

  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read compressed char data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_RAW);
  char *mega = (char *)ctx->buf[BUF_RAW];
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Partition the mega string into individual strings
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < len; i++) {
    SET_STRING_ELT(obj_, i, Rf_mkChar(mega));
    mega += strlen(mega) + 1;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set NA values using the auxilliary NA bistream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unpack_na_str(ctx, BUF_NA_PACKED, obj_, len);
  
  
  UNPROTECT(1);
  return obj_;
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
void write_STRSXP(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // for 'short' STRXP (below the length threshold), just encode as 
  // raw lengths and character data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_length(x_) < ctx->opts->str_threshold) {
    write_STRSXP_raw(ctx, x_);
    return;
  }
  
  switch(ctx->opts->str_transform) {
  case ZAP_STR_RAW:
    write_STRSXP_raw(ctx, x_);
    break;
  case ZAP_STR_MEGA:
    write_STRSXP_mega(ctx, x_);
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
  default:
    Rf_error("read_STRSXP() str transform unknown %i", method);
  }
}




