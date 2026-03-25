

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
#include "utils-packing-nbits.h"
#include "utils-ints.h"

#include "mph.h"



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
#define BUF_DICT         1
#define BUF_MEGA         2
#define BUF_IDX          3
#define BUF_IDX_PACKED   4

void write_STRSXP_dict(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Default to ZAP_STR_MEGA if the string is short
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t len = (size_t)Rf_length(x_);
  if (len <= ctx->opts->str_dict_len_threshold) {
    write_STRSXP_mega(ctx, x_);
    return;
  }
  
  uint32_t max_dict_size = (uint32_t)ceil(ctx->opts->str_dict_frac_limit * (double)len);
  // Rprintf("Max dict size: %.2f * %i = %i\n", ctx->opts->str_dict_frac_limit, (int)len, max_dict_size);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find unique strings using a hashmap
  // Oversize the capacity of the hashmap by a factor of 4 to reduce the 
  // number of collisons + linear probing
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t capacity = max_dict_size * 4;  
  mph_t *mph = mph_init(capacity);
  if (mph == NULL) {
    Rf_error("write_STRSXP_dict_(): Couldn't initialise hashmap");
  }

  // Prep room for the index
  prepare_buf(ctx, BUF_IDX, len * sizeof(int32_t));
  int32_t *dict_idx = (int32_t *)ctx->buf[BUF_IDX];
  
  for (int i = 0; i < len; i++) {
    SEXP chr_ = STRING_ELT(x_, i);
    const char *chr = CHAR(chr_);
    dict_idx[i] = mph_get_set(mph, (uint8_t *)chr, strlen(chr) + 1); // Keep NULL terminator
    if (dict_idx[i] < 0) Rf_error("write_STRSXP_dict_() mph_get_set failed");

    if (mph->nitems > max_dict_size) {
      // Rprintf("Exceeded max dict size (%i) at idx = %i\n", 
              // max_dict_size, i);
      break;
    }
  }
  
  bool can_use_dict = mph->nitems <= max_dict_size;
  // Rprintf("N unique strings: %i (dict = %s)\n", 
  //         (int)mph->nitems, 
  //         can_use_dict ? "Yes" : "No"
  //         );
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If there are too many unique strings, write as a MEGA string
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (!can_use_dict) {
    // Rprintf("STRSXP: Using MEGA string\n");
    mph_destroy(mph);
    write_STRSXP_mega(ctx, x_);
    return;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Otherwise, we can use a dictionary and encode the character vector as
  // an integer vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  prepare_buf(ctx, BUF_DICT, mph->nitems * sizeof(int32_t));
  int32_t *idx_to_bucket = (int32_t *)ctx->buf[BUF_DICT];

  for (int i = 0; i < mph->capacity; i++) {
    bucket_t b = mph->bucket[i];
    if (b.key != NULL) {
      idx_to_bucket[b.value] = i;
    }
  }

  // Create the mega string
  prepare_buf(ctx, BUF_MEGA, mph->total_key_length + 1);
  char *dictp = (char *)ctx->buf[BUF_MEGA];
  
  for (int i = 0; i < mph->nitems; i++) {
    int bucket_idx = idx_to_bucket[i];
    bucket_t b = mph->bucket[bucket_idx];

    strncpy(dictp, (const char *)b.key, b.len);
    dictp += b.len;
  }
  
  // Reset dictp pointer to the start of the mega string
  dictp = (char *)ctx->buf[BUF_MEGA];

  // for (int i = 0; i < total_chars; i++) {
  //   Rprintf("%02x ", dictp[i]);
  // }
  // Rprintf("\n");

  

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
  write_len(ctx, mph->total_key_length);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create the auxilliary bitstream of NA locations
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t packed_len = pack_na_str(ctx, BUF_NA_PACKED, x_);
  write_buf(ctx, BUF_NA_PACKED, packed_len);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Output character data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_len(ctx, mph->nitems);
  write_buf(ctx, BUF_MEGA, mph->total_key_length);


  // write_uint32_buf(ctx, BUF_IDX, len);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many bits can we pack each element into?
  // How many container integers are needed?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t nbits = (size_t)ceil(log2(mph->nitems));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Pack the integers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  packed_len = pack_nbits_ptr_buf(ctx, (uint32_t *)ctx->buf[BUF_IDX], BUF_IDX_PACKED, len, nbits);
  write_buf(ctx, BUF_IDX_PACKED, packed_len);


  mph_destroy(mph);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read STRSXP encoded as a dictionary lookup
// Write:
//  * [1] SEXP
//  * [1] encoding method
//  * [v] number of strings
//  * [v] length of mega string (total_chars)
//  * [bit] boolean vector of location of NA values
//  * [chr] mega string 
//  * [int] integer vector
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_STRSXP_dict(ctx_t *ctx) {
  
  size_t len = read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t)len)); 
  
  if (len == 0) {
    UNPROTECT(1);
    return obj_;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Number of chars in mega-string for dictionary
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t total_chars = read_len(ctx);
  (void)total_chars;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // NA locations
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_NA_PACKED);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read mega-string representing dictionary
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int n_dict_strings = (int)read_len(ctx);
  read_buf(ctx, BUF_MEGA);
  char *mega = (char *)ctx->buf[BUF_MEGA];
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Partition the mega string into individual strings
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP *dict_char = malloc(n_dict_strings * sizeof(SEXP));
  for (int i = 0; i < n_dict_strings; i++) {
    dict_char[i] = PROTECT(Rf_mkChar(mega));
    mega += strlen(mega) + 1;
  }

  // Read the dict indices
  // read_uint32_buf(ctx, BUF_IDX);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many bits are required to encode the dictionary lookup?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t nbits = (size_t)ceil(log2((double)n_dict_strings));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read the compressed data and decompress
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_IDX_PACKED);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the integers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unpack_nbits_buf_ptr(
    ctx, 
    BUF_IDX_PACKED,                 // source (packed)
    (uint32_t *)ctx->buf[BUF_IDX],  // dest   (unpacked)
    len,                            // number of packed ints
    nbits                           // number of bits per int
  );
  
  // Allocate the stirngs
  uint32_t *dict_idx = (uint32_t *)ctx->buf[BUF_IDX];
  for (int i = 0; i < len; i++) {
    SET_STRING_ELT(obj_, i, dict_char[dict_idx[i]]);
  }


  UNPROTECT(n_dict_strings);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set NA values using the auxilliary NA bistream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unpack_na_str(ctx, BUF_NA_PACKED, obj_, len);
  
  UNPROTECT(1);
  return obj_;
}



#undef BUF_NA_PACKED    
#undef BUF_DICT         
#undef BUF_MEGA         
#undef BUF_IDX          
#undef BUF_IDX_PACKED   


