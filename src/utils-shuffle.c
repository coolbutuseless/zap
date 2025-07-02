


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
#include "utils-shuffle.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// A byte shuffle or transpose implementation
//
// Similar to:
//   - https://github.com/powturbo/Turbo-Transpose
// 
// For the deluxe, full-strnegth implementation of this idea see BLOSC
//   - https://www.blosc.org/pages/blosc-in-depth/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Shuffle/Unshuffle bytes in a vector of doubles
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void shuffle8(uint8_t *src, uint8_t *dst, size_t Ndbl) {
  for (int ich = 0; ich < sizeof(double); ++ich) {
    uint8_t *ptr = src + ich;
    for (size_t ip = 0; ip < Ndbl; ++ip) {
      *dst = *ptr;
      ptr += sizeof(double);
      dst += 1;
    }
  }
}


static void unshuffle8(uint8_t *src, uint8_t *dst, size_t Ndbl) {
  for (int ich = 0; ich < sizeof(double); ++ich) {
    uint8_t *dstPtr = dst + ich;
    for (size_t ip = 0; ip < Ndbl; ++ip) {
      *dstPtr = *src;
      src += 1;
      dstPtr += sizeof(double);
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void shuffle_delta8(uint8_t *src, uint8_t *dst, size_t n_dbls) {
  uint8_t prev = 0;
  for (int ich = 0; ich < sizeof(double); ++ich) {
    uint8_t *ptr = src + ich;
    for (size_t ip = 0; ip < n_dbls; ++ip) {
      uint8_t v = *ptr;
      *dst = v - prev;
      prev = v;
      ptr += sizeof(double);  
      dst += 1;
    }
  }
}


static void unshuffle_delta8(uint8_t *src, uint8_t *dst, size_t n_dbls) {
  uint8_t prev = 0;
  for (int ich = 0; ich < sizeof(double); ++ich) {
    uint8_t *dstPtr = dst + ich;
    for (size_t ip = 0; ip < n_dbls; ++ip) {
      uint8_t v = *src + prev;
      prev = v;
      *dstPtr = v;
      src += 1;
      dstPtr += sizeof(double);
    }
  }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void shuffle_delta4(uint8_t *src, uint8_t *dst, size_t n_ints) {
  uint8_t prev = 0;
  for (int ich = 0; ich < sizeof(uint32_t); ++ich) {
    uint8_t *ptr = src + ich;
    for (size_t ip = 0; ip < n_ints; ++ip) {
      uint8_t v = *ptr;
      *dst = v - prev;
      prev = v;
      ptr += sizeof(uint32_t);  
      dst += 1;
    }
  }
}


static void unshuffle_delta4(uint8_t *src, uint8_t *dst, size_t n_ints) {
  uint8_t prev = 0;
  for (int ich = 0; ich < sizeof(uint32_t); ++ich) {
    uint8_t *dstPtr = dst + ich;
    for (size_t ip = 0; ip < n_ints; ++ip) {
      uint8_t v = *src + prev;
      prev = v;
      *dstPtr = v;
      src += 1;
      dstPtr += sizeof(uint32_t);
    }
  }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ###   #               ##     ##    ##            ###  
// #   #  #              #  #   #  #    #           #   # 
// #      # ##   #   #   #      #       #     ###   #   # 
//  ###   ##  #  #   #  ####   ####     #    #   #   ###  
//     #  #   #  #   #   #      #       #    #####  #   # 
// #   #  #   #  #  ##   #      #       #    #      #   # 
//  ###   #   #   ## #   #      #      ###    ###    ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void  shuffle8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls) {
  prepare_buf(ctx, dst_buf, n_dbls * sizeof(double));
  shuffle8(ctx->buf[src_buf], ctx->buf[dst_buf], n_dbls);
} 

void  shuffle8_ptr_buf(ctx_t *ctx, void *src, int dst_buf, size_t n_dbls) {
  prepare_buf(ctx, dst_buf, n_dbls * sizeof(double));
  shuffle8(src, ctx->buf[dst_buf], n_dbls);
} 



void unshuffle8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls) {
  prepare_buf(ctx, dst_buf, n_dbls * sizeof(double));
  unshuffle8(ctx->buf[src_buf], ctx->buf[dst_buf], n_dbls);
}

void unshuffle8_buf_ptr(ctx_t *ctx, int src_buf, void *dst, size_t n_dbls) {
  unshuffle8(ctx->buf[src_buf], dst, n_dbls);
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###   #               ##   ####           ##     #              ###  
//  #   #  #              #  #   #  #           #     #             #   # 
//  #      # ##   #   #   #      #  #   ###     #    ####    ###    #   # 
//   ###   ##  #  #   #  ####    #  #  #   #    #     #         #    ###  
//      #  #   #  #   #   #      #  #  #####    #     #      ####   #   # 
//  #   #  #   #  #  ##   #      #  #  #        #     #  #  #   #   #   # 
//   ###   #   #   ## #   #     ####    ###    ###     ##    ####    ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void  shuffle_delta8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls) {
  prepare_buf(ctx, dst_buf, n_dbls * sizeof(double));
  shuffle_delta8(ctx->buf[src_buf], ctx->buf[dst_buf], n_dbls);
} 

void  shuffle_delta8_ptr_buf(ctx_t *ctx, void *src, int dst_buf, size_t n_dbls) {
  prepare_buf(ctx, dst_buf, n_dbls * sizeof(double));
  shuffle_delta8(src, ctx->buf[dst_buf], n_dbls);
} 



void unshuffle_delta8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls) {
  prepare_buf(ctx, dst_buf, n_dbls * sizeof(double));
  unshuffle_delta8(ctx->buf[src_buf], ctx->buf[dst_buf], n_dbls);
}

void unshuffle_delta8_buf_ptr(ctx_t *ctx, int src_buf, void *dst, size_t n_dbls) {
  unshuffle_delta8(ctx->buf[src_buf], dst, n_dbls);
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###   #               ##   ####           ##     #                #  
//  #   #  #              #  #   #  #           #     #               ##  
//  #      # ##   #   #   #      #  #   ###     #    ####    ###     # #  
//   ###   ##  #  #   #  ####    #  #  #   #    #     #         #   #  #  
//      #  #   #  #   #   #      #  #  #####    #     #      ####   ##### 
//  #   #  #   #  #  ##   #      #  #  #        #     #  #  #   #      #  
//   ###   #   #   ## #   #     ####    ###    ###     ##    ####      #  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void  shuffle_delta4_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints) {
  prepare_buf(ctx, dst_buf, n_ints * sizeof(uint32_t));
  shuffle_delta4(ctx->buf[src_buf], ctx->buf[dst_buf], n_ints);
} 

void  shuffle_delta4_ptr_buf(ctx_t *ctx, void *src, int dst_buf, size_t n_ints) {
  prepare_buf(ctx, dst_buf, n_ints * sizeof(uint32_t));
  shuffle_delta4(src, ctx->buf[dst_buf], n_ints);
} 



void unshuffle_delta4_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints) {
  prepare_buf(ctx, dst_buf, n_ints * sizeof(uint32_t));
  unshuffle_delta4(ctx->buf[src_buf], ctx->buf[dst_buf], n_ints);
}

void unshuffle_delta4_buf_ptr(ctx_t *ctx, int src_buf, void *dst, size_t n_ints) {
  unshuffle_delta4(ctx->buf[src_buf], dst, n_ints);
}


