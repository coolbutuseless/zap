
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
#include "io-ENVSXP.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// There are many different types of environments in R that need
// to be handled individually.
//
// Global, Base & Empty Environments
//   - never serialize this type of environment. Just make a note and
//     re-attach when un-serializing.
//
// Package & Namespace Environments
//   - Record the name of the package/namespace when serializing
//   - Re-attach this named environment when un-serializing
// 
// Full Environment
//   - If the environment isn't one of the above, then 
//       - serialize the parent environment
//       - serialize all members of the environment
//
// The same environment could be included multiple times within a single object
// e.g. ggplot2 nested environments.
//
// So keep references to any environments seen so far, so that we don't 
// serialize them twice, but instead just keep a  reference index and 
// link them all to the same recreated environment when unserializing.
//
// Reference
//   - If a full environment has already been serialized, just save the 
//     index to it.  Re-link to this environment on un-serialization
//   - Note: Two environments are the same if they have the same address
//     in memory
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ENV_GLOBAL    0
#define ENV_BASE      1
#define ENV_EMPTY     2
#define ENV_PACKAGE   3
#define ENV_NAMESPACE 4
#define ENV_REFERENCE 5
#define ENV_FULL      6


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// To find a package environment relies on a non exported API call "R_FindPackageEnv"
// So to mimic this call, call the R function which does the equivalent.
//
// Note: I can't remember where I found this workaround. Mike 2025-05-31
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP my_FindPackageEnv(SEXP nm_) {
  SEXP fun_  = Rf_install("findPackageEnv");
  SEXP expr_ = PROTECT(LCONS(fun_, LCONS(nm_, R_NilValue)));
  SEXP val_  = Rf_eval(expr_, R_GlobalEnv);
  UNPROTECT(1);
  return val_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write environment
//
// Write:
//   sexptype
//   environment type
//   Environment data:
//      - EVN_GLOBAL, ENV_BASE, ENV_EMPTY
//         - nothing else written
//      - ENV_PACKAGE, ENV_NAMESPACE
//         - name (string)
//      - ENV_REFERENCE
//         - index (integer)
//      - ENV_FULL
//         - parent environment
//         - all elem names
//         - for each elem:
//             -write elem
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_ENVSXP(ctx_t *ctx, SEXP env_) {
  
  write_uint8(ctx, ENVSXP);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Is it one of the standard environments? 
  // Just write the environment type and name
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (env_ == R_GlobalEnv) {
    write_uint8(ctx, ENV_GLOBAL);
    return;
  } else if (env_ == R_BaseEnv) {
    write_uint8(ctx, ENV_BASE);
    return;
  } else if (env_ == R_EmptyEnv) {
    write_uint8(ctx, ENV_EMPTY);
    return;
  } else if (R_IsPackageEnv(env_)) {
    write_uint8(ctx, ENV_PACKAGE);
    SEXP env_name_ = PROTECT(R_PackageEnvName(env_));
    write_sexp(ctx, env_name_);
    UNPROTECT(1);
    return;
  } else if (R_IsNamespaceEnv(env_)) {
    write_uint8(ctx, ENV_NAMESPACE);
    SEXP env_spec_ = PROTECT(R_NamespaceEnvSpec(env_));
    write_sexp(ctx, env_spec_);
    UNPROTECT(1);
    return;
  } 
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Has this environment been seen in the hashmap cache?
  // If so, just return an environment reference
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int hash_idx = mph_lookup(ctx->envsxp_hashmap, (uint8_t *)&env_, 8);
  if (hash_idx >= 0) {
    // Found the environment in the cache
    write_uint8(ctx, ENV_REFERENCE);
    write_len(ctx, (uint64_t)hash_idx);
    return;
  }

  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If we got this far, then the environment must one which needs to be 
  // fully serialized!
  //
  // Store reference to this environment so we can see if any future
  // environments are an exact match
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  hash_idx = mph_add(ctx->envsxp_hashmap, (uint8_t *)&env_, 8);
  if (hash_idx != ctx->Nenv) {
    Rf_error("write_ENVSXP(): Hashmap synchronization error %i != %i", (int)hash_idx, (int)ctx->Nenv);
  }
  ctx->Nenv++;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // This is a 'full' Environment
  // - write the parent
  // - iterate over the contents and write each
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_uint8(ctx, ENV_FULL);
  write_sexp(ctx, R_ParentEnv(env_));
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find all names in the environment.
  // Write the names
  // 'get' each name
  // write it out.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(R_lsInternal3(env_, true, false));
  R_xlen_t len = Rf_xlength(nms_);
  write_sexp(ctx, nms_);

  for (R_xlen_t i = 0; i < len; i++) {
    SEXP nm_ = PROTECT(Rf_installChar(STRING_ELT(nms_, i)));
    // Rprintf(">>>> %s\n", CHAR(STRING_ELT(nms_, i)));
    if (Rf_findVar(nm_, env_) == R_MissingArg) {
      // Rprintf("Environment write. Missing var: '%s'\n", CHAR(STRING_ELT(nms_, i)));
      write_sexp(ctx, R_MissingArg);
      UNPROTECT(1);
    } else {
      SEXP el_ = PROTECT(R_getVar(nm_, env_, TRUE));
      write_sexp(ctx, el_);
      UNPROTECT(2);
    }
  }
  
  
  UNPROTECT(1);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read environment
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_ENVSXP(ctx_t *ctx) {
  
  int env_type = read_uint8(ctx);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Is it a standard environment or reference?  Re-create the link
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (env_type == ENV_GLOBAL) {
    return R_GlobalEnv;
  } else if (env_type == ENV_BASE) {
    return R_BaseEnv;
  } else if (env_type == ENV_EMPTY) {
    return R_EmptyEnv;
  } else if (env_type == ENV_PACKAGE) {
    SEXP nm_ = PROTECT(read_sexp(ctx));
    SEXP env_ = PROTECT(my_FindPackageEnv(nm_));
    UNPROTECT(2);
    return env_;
  } else if (env_type == ENV_NAMESPACE) {
    SEXP nm_ = PROTECT(read_sexp(ctx));
    SEXP env_ = PROTECT(R_FindNamespace(nm_));
    UNPROTECT(2);
    return env_;
  } else if (env_type == ENV_REFERENCE) {
    R_xlen_t env_idx = (R_xlen_t)read_len(ctx);
    SEXP env_list_ = VECTOR_ELT(ctx->cache, ZAP_CACHE_ENVSXP);
    return VECTOR_ELT(env_list_, env_idx);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If it wasn't just a standard environment, or a reference environment, 
  // then it MUST be a full environment. Check!
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (env_type != ENV_FULL) {
    Rprintf("read_ENVSXP(): Expecting full environment but got [%i]", env_type);
  }
  
  int nprotect = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read the parent.
  // Create this environment
  //    R_NewEnv(SEXP parent, int hash, int size)
  // hash: is this a hashed environment?
  // size: initial hash size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP parent_ = PROTECT(read_sexp(ctx)); nprotect++;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Remember this environment for  references
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP env_    = PROTECT(R_NewEnv(parent_, true, 29)); nprotect++;
  
  SEXP env_list_ = VECTOR_ELT(ctx->cache, ZAP_CACHE_ENVSXP);
  SET_VECTOR_ELT(env_list_, ctx->Nenv, env_);
  ctx->Nenv++;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // If we've reached the limit of current env cache, then expand the cache
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ctx->Nenv >= Rf_xlength(env_list_)) {
    SEXP expanded_env_list_ = PROTECT(Rf_lengthgets(env_list_, 2 * Rf_length(env_list_)));
    SET_VECTOR_ELT(ctx->cache, ZAP_CACHE_ENVSXP, expanded_env_list_);
    UNPROTECT(1);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read the names for objects in this environment
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nms_ = PROTECT(read_sexp(ctx)); nprotect++;
  R_xlen_t len = Rf_xlength(nms_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Read the objects in this environment and assign
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (R_xlen_t i = 0; i < len; i++) {
    SEXP el_ = PROTECT(read_sexp(ctx));
    SEXP nm_ = PROTECT(Rf_installChar(STRING_ELT(nms_, i)));
    Rf_defineVar(nm_, el_, env_);
    UNPROTECT(2);
  }
  
  
  UNPROTECT(nprotect);
  return env_;
}



