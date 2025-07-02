
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


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Pack smaller integer values into a 64-bit integer
//
// Notes:
//   - uint64_t was chosen for simplicity of initial implementation
//   - nbits-per-value    Num values in a uin64_t     wasted bits per uint64_t
//     1                  64                           0
//     2                  32                           0
//     3                  21                           1
//     4                  16                           0
//     5                  12                           4
//     6                  10                           4
//     7                   9                           1
//     8                   8                           0
//     9                   7                           0
//    10                   6                           4
//    11                   5                           9
//    12                   5                           4
//
//  With the target being uin64_t storage, its only worth storing up to 12-bit
//  integers.  For higher nbits, just switch to zzshuf style coding
//    
//
// References
//  - https://github.com/fast-pack/LittleIntPacker
//  - https://lemire.me/blog/2012/03/06/how-fast-is-bit-packing/
//
// Future:
//  - this could be swapped out for a general purpose bit-packer which 
//    will pack across byte boundaries and not waste space
//  - experiment with calculating delta between consecutive elements
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~




#define PACK_TYPE  uint64_t  // Storage container

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Calculate the number of uint64_t integers to hold a vector of integers which
// are each packed into 'nbits' per value
// This includes padding bits to have the result round-up to the nearest
// multiple of 4
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t calc_num_container_ints(size_t Nints, size_t nbits) {
  size_t pack_size = (size_t)floor(sizeof(PACK_TYPE) * 8.0 / (double)nbits);
  size_t n_container_ints = (size_t)ceil((double)Nints / (double)pack_size);
  return n_container_ints;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Pack a vector of uint32s into uint64s
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t pack_nbits_ptr_ptr(ctx_t *ctx, uint32_t *src, void *dst, size_t N_orig_ints, size_t nbits) {
  
  size_t n_container_ints = calc_num_container_ints(N_orig_ints, nbits);
  size_t packed_len = n_container_ints * sizeof(PACK_TYPE);
  
  PACK_TYPE *p = dst;
  int n_ints_per_packing = (int)floor((double)(sizeof(PACK_TYPE) * 8) / (double)nbits);
  PACK_TYPE mask = (1ULL << nbits) - 1;
  
  // Rprintf("pack: nbits = %i, ints-per-pack = %i\n", nbits, n_ints_per_packing);
  
  int i = 0;
  for (; i < (int64_t)N_orig_ints - (n_ints_per_packing - 1); i += n_ints_per_packing) {
    *p = 0;
    int shift = nbits * (n_ints_per_packing - 1);
    for (int j = 0; j < n_ints_per_packing; j++) {
      *p |= ((src[i + j] & mask) << shift);
      shift -= nbits;
    }
    p++;
  }
  
  if (i < N_orig_ints) {  
    *p = 0;
    int shift = nbits * (n_ints_per_packing - 1);
    for (;i < N_orig_ints; i++) {
      *p |= (PACK_TYPE)(src[i] & mask) << shift;
      shift -= nbits;
    }
    // Rprintf("p <- 0x%x\n", *p);
  }
  
  return packed_len;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void unpack_nbits_ptr_ptr(ctx_t *ctx, void *src, uint32_t *dst, size_t N_orig_ints, size_t nbits) {
  
  PACK_TYPE *p = src;
  int n_ints_per_packing = (int)floor((double)(sizeof(PACK_TYPE) * 8) / (double)nbits);
  PACK_TYPE mask = (1ULL << nbits) - 1;
  
  int i = 0;
  for (; i < (int64_t)N_orig_ints - (n_ints_per_packing - 1); i += n_ints_per_packing) {
    int shift = nbits * (n_ints_per_packing - 1);
    for (int j = 0; j < n_ints_per_packing; j++) {
      dst[i + j] = (uint32_t)(*p >> shift) & mask;
      shift -= nbits;
    }
    p++;
  }
  
  if (i < N_orig_ints) {
    int shift = nbits * (n_ints_per_packing - 1);
    for (; i < N_orig_ints; i++) {
      dst[i] = (uint32_t)(*p >> shift) & mask;
      shift -= nbits;
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t pack_nbits_ptr_buf(ctx_t *ctx, uint32_t *src, int buf_dst, size_t N_orig_ints, size_t nbits) {
  
    size_t n_container_ints = calc_num_container_ints(N_orig_ints, nbits);
    size_t packed_len = n_container_ints * sizeof(PACK_TYPE);
    prepare_buf(ctx, buf_dst, packed_len);
  
    return pack_nbits_ptr_ptr(ctx, src, (void *)ctx->buf[buf_dst], N_orig_ints, nbits);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void unpack_nbits_buf_ptr(ctx_t *ctx, int buf_src, uint32_t *dst, size_t N_orig_ints, size_t nbits) {
  
  unpack_nbits_ptr_ptr(
    ctx, 
    (void *)ctx->buf[buf_src],    // source (packed)
    dst,                          // dest   (unpacked)
    N_orig_ints,                  // number of packed ints
    nbits                         // number of bits per int
  );
  
}




