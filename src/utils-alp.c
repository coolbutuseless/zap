
#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include "io-ctx.h"
#include "utils-alp.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This method is adapted from a *Afroozeh et al* 
// [ALP: Adaptive Lossless floating-Point Compression](https://dl.acm.org/doi/pdf/10.1145/3626717).
// The original [C++ code on github](https://github.com/cwida/ALP) and the 
// [Rust implementation](https://github.com/spiraldb/alp) are available. 
// Note: [the rust version is faster than c](https://spiraldb.com/post/alp-rust-is-faster-than-c)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static double fact[24] = {
  1.0,
  10.0,
  100.0,
  1000.0,
  10000.0,
  100000.0,
  1000000.0,
  10000000.0,
  100000000.0,
  1000000000.0,
  10000000000.0,
  100000000000.0,
  1000000000000.0,
  10000000000000.0,
  100000000000000.0,
  1000000000000000.0,
  10000000000000000.0,
  100000000000000000.0,
  1000000000000000000.0,
  10000000000000000000.0,
  100000000000000000000.0,
  1000000000000000000000.0,
  10000000000000000000000.0,
  100000000000000000000000.0 
};

static double invfact[24] = {
  1.0,
  0.1,
  0.01,
  0.001,
  0.0001,
  0.00001,
  0.000001,
  0.0000001,
  0.00000001,
  0.000000001,
  0.0000000001,
  0.00000000001,
  0.000000000001,
  0.0000000000001,
  0.00000000000001,
  0.000000000000001,
  0.0000000000000001,
  0.00000000000000001,
  0.000000000000000001,
  0.0000000000000000001,
  0.00000000000000000001,
  0.000000000000000000001,
  0.0000000000000000000001,
  0.00000000000000000000001  
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// C version of fast_round
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double fast_round(double x) {
  double sweet = 6755399441055744.0;  // 2^51 + 2^52
  return x + sweet - sweet;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Check for special values which are impossible for ALP to encode
// because they cannot be cast to int64 without an undefined behaviour
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ENCODING_UPPER_LIMIT   9223372036854774784
#define ENCODING_LOWER_LIMIT  -9223372036854774784

bool is_impossible_to_encode(double n) {
  return 
  !isfinite(n) || 
    isnan(n) || 
    n > ENCODING_UPPER_LIMIT || 
    n < ENCODING_LOWER_LIMIT ||
    (n == 0.0 && signbit(n)); //! Verification for -0.0
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Probe the given double vector by
//  -  sampling N equi-spaced values
//  -  finding the best params for this sampling
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alp_params_t alp_probe(double *x, size_t len, size_t N) {
  alp_params_t aparams = {
    .e     =   0,
    .f     =   0,
    .score =  -1,
    .gap   = 999
  };

  
  size_t delta;  
  if (N > len) {
    N = len;
    delta = 1;  
  } else {
    delta = (size_t)floor((double)len / (double)N);
  }
  
  // Rprintf("Len: %i  N: %i  Delta: %i\n", len, N, delta);
  
  for (int e = 15; e >= 0; e--) {
    for (int f = e; f >= 0; f--) {
      int score = 0;
      
      size_t i = 0;
      for (size_t j = 0; j < N; j++) {
        if (!is_impossible_to_encode(x[i])) {
          double renc = fast_round(x[i] * fact[e] * invfact[f]);
          if (renc < ENCODING_UPPER_LIMIT && renc > ENCODING_LOWER_LIMIT) {
            int64_t enc = (int64_t)(renc);
            double dec  = (double)enc               * fact[f] * invfact[e];
            score += (dec == x[i] && signbit(dec) == signbit(x[i]));
          }
        }
        i += delta;
      }
      
      int gap = e - f;
      
      if ((score > aparams.score) || ((score == aparams.score) && (gap < aparams.gap))) {
        aparams.score = score;
        aparams.gap   = gap;
        aparams.e     = e;
        aparams.f     = f;
      }
    }
  }
  
  aparams.ntest = N;
  return aparams;
}


// # nocov start
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// For debugging - this function calculates the best e/f across ALL values
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alp_params_t alp_probe_full(double *x, size_t len) {
  alp_params_t aparams = {
    .e     =   0,
    .f     =   0,
    .score =  -1,
    .gap   = 999
  };
  
  for (int e = 15; e >= 0; e--) {
    for (int f = e; f >= 0; f--) {
      int score = 0;
      
      for (size_t i = 0; i < len; i++) {
        if (is_impossible_to_encode(x[i])) continue;
        double renc = fast_round(x[i] * fact[e] * invfact[f]);
        if (renc < ENCODING_UPPER_LIMIT && renc > ENCODING_LOWER_LIMIT) {
          int64_t enc = (int64_t)(fast_round(x[i] * fact[e] * invfact[f]));
          double dec  = (double)enc               * fact[f] * invfact[e];
          score += (dec == x[i] && signbit(dec) == signbit(x[i]));
        }
      }
      
      int gap = e - f;
      
      if ((score > aparams.score) || ((score == aparams.score) && (gap < aparams.gap))) {
        aparams.score = score;
        aparams.gap   = gap;
        aparams.e     = e;
        aparams.f     = f;
      }
    }
  }
  
  return aparams;
}
// # nocov end

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Encode double precision values with ALP
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void alp_encode(double *src, int64_t *dst, size_t len, int e, int f, double *patch, 
                  uint32_t *patch_idx, uint32_t *npatch) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Encode subsequent values 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (size_t i = 0; i < len; i++) {
    if (is_impossible_to_encode(src[i])) {
      dst[i] = (i == 0) ? 0 : dst[i - 1];
      
      patch_idx[*npatch] = (uint32_t)i;
      patch[*npatch] = src[i];
      *npatch += 1;
      
      continue;
    }
    int64_t enc = (int64_t)(fast_round(src[i] * fact[e] * invfact[f]));
    double dec  = enc                         * fact[f] * invfact[e];
    if (dec != src[i]) {
      patch_idx[*npatch] = (uint32_t)i;
      patch[*npatch] = src[i];
      *npatch += 1;
      dst[i] = (i == 0) ? 0 : dst[i - 1];
    } else {
      dst[i] = enc; 
    }
  }
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Decode ALP values
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void alp_decode(int64_t *src, double *dst, size_t len, int e, int f, double *patch, 
                uint32_t *patch_idx, uint32_t npatch) {
  
  // Decode values
  for (size_t i = 0; i < len; i++) {
    dst[i] = (double)src[i] * fact[f] * invfact[e];
  }
  
  // Patch exceptions
  for (size_t i = 0; i < npatch; i++) {
    dst[patch_idx[i]] = patch[i];
  }
}



