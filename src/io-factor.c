
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
#include "io-factor.h"
#include "utils-packing-nbits.h"


#define BUF_PACKED     0
#define BUF_RAW        1

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a factor
//
// If nlevels < 2^12
//    - write as packed integers in uint64_t
//    - this is really just a custom frame-of-reference encoding
//
// Otherwise 
//    - write as an integer
//
// The factor levels themselves are part of the attributes on the object
// and are not handled here.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_factor(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Factor levels are positive integers and NA
  // When NA is masked to fit into 'N' bits, it will be zero 
  //   i.e. packed values of zero are equivalent to NA values
  // E.g. if factor has 4 levels, then will need 3bits to encode
  //      0    000  NA
  //      1    001
  //      2    010
  //      3    011
  //      4    100
  //
  // So the number of levels which need to be available = 
  //     * number of levels in factor
  //     * a code for 'zero' which holds the NAs
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int nlevels = (int)Rf_length(Rf_getAttrib(x_, R_LevelsSymbol)) + 1; 
  size_t len = (size_t)Rf_xlength(x_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Only packing factors if they have levels which fit into 12 bits
  // Otherwise just write out as an integer vector.
  // Why 12 bits?  Because 12-bit ints are the largest ints which get any 
  // benefit from being packed into a 64-bit container.
  // Possible Future: Implement unbounded packing which spans boundaries
  // of the container type.  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ctx->opts->fct_transform == ZAP_FCT_RAW || nlevels >= (1 << 12) || len < ctx->opts->fct_threshold) {
    write_INTSXP_raw(ctx, x_);
    return;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Write:
  //   - sexptype
  //   - nlevels - total number of levels to encode. including zero
  //   - length (number of elements)
  //   - data - packed integers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_uint8(ctx, FACTORSXP);
  write_len(ctx, (uint64_t) nlevels);
  write_len(ctx, (uint64_t)len);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Early return 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (len == 0) return;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many bits can we pack each element into?
  // How many container integers are needed?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t nbits = (size_t)ceil(log2(nlevels));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Pack the integers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t packed_len = pack_nbits_ptr_buf(ctx, (uint32_t *)INTEGER(x_), BUF_PACKED, len, nbits);
  write_buf(ctx, BUF_PACKED, packed_len);
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read in a packed factor
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_factor(ctx_t *ctx) {
  
  size_t nlevels = read_len(ctx); // factor_levels + 1
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialise factor - it's just an INTSXP vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t len = (size_t)read_len(ctx);
  SEXP x_ = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t)len)); 
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Early return
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (len == 0) {
    UNPROTECT(1);
    return x_;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many container ints are expected?
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t nbits = (size_t)ceil(log2((double)nlevels));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read the compressed data and decompress
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  read_buf(ctx, BUF_PACKED);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the integers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  unpack_nbits_buf_ptr(
    ctx, 
    BUF_PACKED,               // source (packed)
    (uint32_t *)INTEGER(x_),  // dest   (unpacked)
    len,                      // number of packed ints
    nbits                     // number of bits per int
  );
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Anything which is zero is really an NA
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int32_t *p = INTEGER(x_);
  for (int i = 0; i < len; i++) {
    if (p[i] == 0) p[i] = NA_INTEGER;
  }
  
  
  UNPROTECT(1);
  return x_;
}



