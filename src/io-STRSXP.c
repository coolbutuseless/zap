

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

#include "mph.h"

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
// read MEGA
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
  // Read char data
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
// Dictionary lookup
//
// When there are only a few distinct elements in the string vector,
// then encode the unique strings as a dictionary, and encode the 
// string vector as an integer index into the dictionary.
//
// String vectors with only a small number of distinct elements.
// Transmit:
//   * the number of strings
//   * the bit-packed vector of boolean values indicating "is NA"
//   * the number of unique strings
//   * the length of the dictionary
//   * the cumulative offsets and lengths of the strings in the dictionary
//   * the dictionary of unique strings
//   * a vector of integers with the index of each string in the dictionary
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUF_NA_PACKED    0
#define BUF_RAW          1
#define BUF_COMP         2

#define MAX_DICT_SIZE    4

void write_STRSXP_dict(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Default to ZAP_STR_MEGA if the string is short
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t len = (size_t)Rf_length(x_);
  // if (len == 0 || len < 32) {
  //   write_STRSXP_mega(ctx, x_);
  //   return;
  // }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find unique strings using a hashmap
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t capacity = MAX_DICT_SIZE * 4;
  mph_t *mph = mph_init(capacity);
  if (mph == NULL) {
    Rf_error("write_STRSXP_dict_(): Couldn't initialise hashmap");
  }

  // Prep room for the index
  int32_t *dict_idx = malloc(len * sizeof(int32_t));
  if (dict_idx == NULL) Rf_error("write_STRSXP_dict_(): Couldn't initialise integer idx");
  
  for (int i = 0; i < len; i++) {
    SEXP chr_ = STRING_ELT(x_, i);
    const char *chr = CHAR(chr_);
    dict_idx[i] = mph_get_set(mph, (uint8_t *)chr, strlen(chr) + 1); // Keep NULL terminator
    if (dict_idx[i] < 0) Rf_error("write_STRSXP_dict_() mph_get_set failed");

    if (mph->nitems > MAX_DICT_SIZE) {
      Rprintf("Exceeded max dict size (%i) at idx = %i\n", 
              MAX_DICT_SIZE, i);
      break;
    }
  }
  
  bool can_use_dict = mph->nitems <= MAX_DICT_SIZE;
  Rprintf("N unique strings: %i (dict = %s)\n", 
          (int)mph->nitems, 
          can_use_dict ? "Yes" : "No"
          );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If there are too many unique strings, write as a MEGA string
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!can_use_dict) {
    Rprintf("STRSXP: Using MEGA string\n");
    free(dict_idx);
    mph_destroy(mph);
    write_STRSXP_mega(ctx, x_);
    return;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Otherwise, we can use a dictionary and encode the character vector as
  // an integer vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Rprintf("STRSXP: Using Dict\n");
  uint64_t total_chars = 0;
  for (int i = 0; i < mph->capacity; i++) {
    bucket_t b = mph->bucket[i];
    if (b.key != NULL) {
      Rprintf("[%i] %s\n", i, b.key);
      total_chars += strlen((char *)b.key) + 1; // count NUL terminator byte
    }
  }
  Rprintf("TOTAL: %i\n", (int)total_chars);

  for (int i = 0; i < len; i++) {
    Rprintf("[%i] %i\n", i, dict_idx[i]);
  }
  
  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write:
  //  * [1] SEXP
  //  * [1] encoding method
  //  * [v] number of strings
  //  * [v] length of mega string (total_chars)
  //  * [bit] boolean vector of location of NA values
  //  * [chr] mega string 
  //  * [int] integer vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_uint8(ctx, STRSXP);
  write_uint8(ctx, ZAP_STR_DICT);
  write_len(ctx, len);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find lengths of all strings. (Including NULL byte)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  total_chars = 0;
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

  
  free(dict_idx);
  mph_destroy(mph);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read STRSXP encoded as a dictionary lookup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_STRSXP_dict(ctx_t *ctx) {
  
  Rf_error("read_STRSXP_dict(): not done yet");
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




