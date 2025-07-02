


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
#include "utils-packing-nbits.h"
#include "utils-int-frame-delta.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Encode an integer vector be tracking:
//   * initial value
//   * difference between consecutive elements
//   * map all these differences to be positive
//   * if maximum mapped difference > 4096 (12 bits), then switch to zzshuf
//   * pack these differences with minimal nbits into uint64_t
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



// void printBits(size_t const size, void const * const ptr)
// {
//   unsigned char *b = (unsigned char*) ptr;
//   unsigned char byte;
//   int i, j;
//   
//   for (i = size-1; i >= 0; i--) {
//     for (j = 7; j >= 0; j--) {
//       byte = (b[i] >> j) & 1;
//       Rprintf("%u", byte);
//     }
//   }
//   Rprintf("\n");
// }


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Compress from src to dst
//
// @param src,dst are pre-allocated memory of the appropriates sizes
// @param src_len the src buffer has this many bytes of data to compress
// 
// @return The number of bytes of compressed data in dst
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t deltaframe_encode_ptr_ptr(ctx_t *ctx, void *src, void *dst, size_t n_ints, int32_t *ref, int32_t *delta_offset, size_t *nbits) {
  
  int32_t *psrc = (int32_t *)src;
  *nbits = 32;
  *ref   = NA_INTEGER;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find first Non-NA value
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < n_ints; i++) {
    if (psrc[i] != NA_INTEGER) {
      *ref = psrc[i];
      break;
    }
  }
  
  if (*ref == NA_INTEGER) {
    // All values are NA, so we won't use deltaframe encoding
    *nbits = 32; // indicate failure
    return 0;
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Calculate min/max delta between elements
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int32_t min_delta = 0;
  int32_t max_delta = 0;
  
  int32_t prior = *ref;
  for (int i = 1; i < n_ints; i++) {
    if (psrc[i] == NA_INTEGER) continue;
    
    int32_t delta = psrc[i] - prior;
    if (delta < min_delta) min_delta = delta;
    if (delta > max_delta) max_delta = delta;
    
    prior = psrc[i];
  }
  
  uint64_t range64 = (uint64_t)max_delta - (uint64_t)min_delta + 2; 
  *nbits = (size_t)ceil(log2(range64));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // How many bits to encode the range?  (ignoring NA value)
  // Values are being packed into 64-bit unsigned integers, and 12bit ints
  // are the largest integer which get any benefit from this
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if ( range64 > 4096 || *nbits > 12) {
    *nbits = 32; // indicate failure
    return 0;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Calculate the deltas between consecutive elements.
  // Subtrack the minimum delta so that the deltas are all non-negative
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint32_t *tmp = malloc(n_ints * sizeof(uint32_t));
  if (tmp == NULL) Rf_error("deltaframe_encode_ptr_ptr() malloc(tmp) failed");
  tmp[0] = 0;
  
  prior = *ref;
  for (int i = 1; i < n_ints; i++) {
    if (psrc[i] == NA_INTEGER) {
      // For NAs, set diff to 0.  I.e. LOCF
      tmp[i] = (uint32_t)(0 - min_delta); // abusing wrap-around for unsigned ints
      continue;
    }
    tmp[i] = (uint32_t)((psrc[i] - prior) - min_delta);
    prior = psrc[i];
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Pack the non-negative deltas into n-bits each
  // packed_len = num bytes written to 'dst'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t packed_len = pack_nbits_ptr_ptr(ctx, tmp, dst, n_ints, *nbits);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Record the offset applied to each delta.  This will be un-applied
  // when reading the data back
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  *delta_offset = min_delta;
  
  free(tmp);
  return packed_len;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Compress from the src_buf to the dst_buf
//
// @param src_buf,dst_buf integer indices for the buffers in 'ctx'
// @param src_len the src buffer has this many bytes of data to compress
// 
// @return The number of bytes of compressed data in dst_buf
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t deltaframe_encode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints, int32_t *ref, int32_t *delta_offset, size_t *nbits) {
  
  prepare_buf(ctx, dst_buf, n_ints * sizeof(int32_t));
  size_t nbytes = deltaframe_encode_ptr_ptr(ctx, ctx->buf[src_buf], ctx->buf[dst_buf], n_ints, ref, delta_offset, nbits);
  
  return nbytes;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Compress from the src_buf to the dst_buf
//
// @param src_buf,dst_buf integer indices for the buffers in 'ctx'
// @param src_len the src buffer has this many bytes of data to compress
// 
// @return The number of bytes of compressed data in dst_buf
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t deltaframe_encode_ptr_buf(ctx_t *ctx, void *src, int dst_buf, size_t n_ints, int32_t *ref, int32_t *delta_offset, size_t *nbits) {
  
  prepare_buf(ctx, dst_buf, n_ints * sizeof(int32_t));
  size_t nbytes = deltaframe_encode_ptr_ptr(ctx, src, ctx->buf[dst_buf], n_ints, ref, delta_offset, nbits);
  
  return nbytes;
}








//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void deltaframe_decode_ptr_ptr(ctx_t *ctx, void *src, void *dst, size_t n_ints, int32_t ref, int32_t delta_offset, size_t nbits) {
  
  int32_t *pdst = (int32_t *)dst;
  unpack_nbits_ptr_ptr(ctx, src, dst, n_ints, nbits);
  
  pdst[0] = ref;
  for (int i = 1; i < n_ints; i++) {
    pdst[i] = pdst[i - 1] + pdst[i] + delta_offset;
  }
}


// @param src_buf is already correctly allocated and holding 'src_len' bytes
//        of compressed data
// @param dst_buf for the decompressed bytes
void deltaframe_decode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints, int32_t ref, int32_t delta_offset, size_t nbits) {
  
  prepare_buf(ctx, dst_buf, n_ints * sizeof(int32_t)); // maximum possible size
  deltaframe_decode_ptr_ptr(ctx, ctx->buf[src_buf], ctx->buf[dst_buf], n_ints, ref, delta_offset, nbits);
}


// @param src_buf is already correctly allocated and holding 'src_len' bytes
//        of compressed data
// @param dst_buf for the decompressed bytes
void deltaframe_decode_buf_ptr(ctx_t *ctx, int src_buf, void *dst, size_t n_ints, int32_t ref, int32_t delta_offset, size_t nbits) {
  deltaframe_decode_ptr_ptr(ctx, ctx->buf[src_buf], dst, n_ints, ref, delta_offset, nbits);
}










