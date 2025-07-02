


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
#include "utils-zigzag.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ZigZag encoding
// https://lemire.me/blog/2022/11/25/making-all-your-integers-positive-with-zigzag-encoding/
// to recode integers without a sign bit
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  #####  #####         #####               
//      #      #         #                   
//     #      #          #      # ##    ###  
//    #      #           ####   ##  #  #   # 
//   #      #            #      #   #  #     
//  #      #             #      #   #  #   # 
//  #####  #####         #####  #   #   ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void zigzag_encode_ptr_ptr(void *src, void *dst, size_t n_ints) {
  
  uint32_t *in  = (uint32_t *)src;
  uint32_t *out = (uint32_t *)dst;
  
  for(size_t i = 0; i < n_ints; i++) {
    out[i] = (in[i] + in[i]) ^ (uint32_t)((int32_t)in[i] >> 31);
  }
  
}

void zigzag_encode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints) {
  prepare_buf(ctx, dst_buf, n_ints * sizeof(int));
  zigzag_encode_ptr_ptr(ctx->buf[src_buf], ctx->buf[dst_buf], n_ints);
}

void zigzag_encode_ptr_buf(ctx_t *ctx, void *src, int dst_buf, size_t n_ints) {
  prepare_buf(ctx, dst_buf, n_ints * sizeof(int));
  zigzag_encode_ptr_ptr(src, ctx->buf[dst_buf], n_ints);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  #####  #####         ####                
//      #      #          #  #               
//     #      #           #  #   ###    ###  
//    #      #            #  #  #   #  #   # 
//   #      #             #  #  #####  #     
//  #      #              #  #  #      #   # 
//  #####  #####         ####    ###    ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void zigzag_decode_ptr_ptr(void *src, void *dst, size_t n_ints) {
  
  uint32_t *in  = (uint32_t *)src;
  uint32_t *out = (uint32_t *)dst;
  
  for (size_t i = 0; i < n_ints; i++) {
    out[i] = (in[i] >> 1) ^ (0-(in[i] & 1));
  }
}

void zigzag_decode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints) {
  prepare_buf(ctx, dst_buf, (size_t)n_ints * sizeof(int));
  zigzag_decode_ptr_ptr(ctx->buf[src_buf], ctx->buf[dst_buf], n_ints);
}

void zigzag_decode_buf_ptr(ctx_t *ctx, int src_buf, void *dst, size_t n_ints) {
  zigzag_decode_ptr_ptr(ctx->buf[src_buf], dst, n_ints);
}








