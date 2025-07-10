
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
#include "io-core.h"
#include "io-VECSXP.h"


#define VECSXP_RAW       0
#define VECSXP_REFERENCE 1


// #define USE_CACHE 1

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write a stnadard R list
//
// Write:
//   sexptype
//   length
//   for each elem:
//      write elem
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_VECSXP(ctx_t *ctx, SEXP x_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Has this VECSXP been seen in the hashmap cache?
  // If so, just return an VECSXP reference
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef USE_CACHE
    int hash_idx = mph_lookup(ctx->vecsxp_hashmap, (uint8_t *)&x_, 8);
    if (hash_idx >= 0) {
      // Found the VECSXP in the cache
      write_uint8(ctx, VECSXP);
      write_uint8(ctx, VECSXP_REFERENCE);
      write_len(ctx, (uint64_t)hash_idx);
      return;
    }
    
    hash_idx = mph_add(ctx->vecsxp_hashmap, (uint8_t *)&x_, 8);
    
    if (hash_idx != ctx->Nvecsxp) {
      Rf_error("write_VECSXP() hashmap sync error %i != %i",
               hash_idx, (int)ctx->Nvecsxp);
    }
    
    ctx->Nvecsxp++;
    
    write_uint8(ctx, VECSXP);
    write_uint8(ctx, VECSXP_RAW);
#else
    write_uint8(ctx, VECSXP);
#endif 
  

  R_xlen_t len = Rf_xlength(x_);
  write_len(ctx, (uint64_t)len);
  
  for (R_xlen_t i = 0; i < len; i++) {
    write_sexp(ctx, VECTOR_ELT(x_, i));
  }
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read a standard R list
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_VECSXP(ctx_t *ctx) {
  
#ifdef USE_CACHE
  int type = read_uint8(ctx);
  if (type == VECSXP_REFERENCE) {
    uint64_t hash_idx = (R_xlen_t)read_len(ctx);
    SEXP vecsxp_list_ = VECTOR_ELT(ctx->cache, ZAP_CACHE_VECSXP);
    return VECTOR_ELT(vecsxp_list_, hash_idx);
  }
#endif
  
  
  R_xlen_t len = (R_xlen_t)read_len(ctx);
  SEXP obj_ = PROTECT(Rf_allocVector(VECSXP, len)); 
  
  
#ifdef USE_CACHE
  SEXP vecsxp_list_ = VECTOR_ELT(ctx->cache, ZAP_CACHE_VECSXP);
  SET_VECTOR_ELT(vecsxp_list_, ctx->Nvecsxp, obj_);
  ctx->Nvecsxp++;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If we've reached the limit of current cache, then expand the cache
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ctx->Nvecsxp >= Rf_xlength(vecsxp_list_)) {
    SEXP expanded_vecsxp_list_ = PROTECT(Rf_lengthgets(vecsxp_list_, 2 * Rf_length(vecsxp_list_)));
    SET_VECTOR_ELT(ctx->cache, ZAP_CACHE_VECSXP, expanded_vecsxp_list_);
    UNPROTECT(1);
  }
#endif
  
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP el_ = PROTECT(read_sexp(ctx));
    SET_VECTOR_ELT(obj_, i, el_);
    UNPROTECT(1);
  }
  
  
  UNPROTECT(1);
  return obj_;
}

