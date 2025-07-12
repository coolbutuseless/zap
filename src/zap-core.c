
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

#include "utils-df.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The specific context data for serialising/unserializing
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  size_t pos;
  size_t capacity;
  uint8_t *data;
} raw_buffer_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Header length = 4 bytes
// [0] Z & 0x80
// [1] Version number (uint8_t)
// [2] flags1
// [3] flags2
//
// Version 1
//   - Initial release 0.1.0
// Version 2
//   - v0.1.1 2025-07-11
//   - add 2 extra flag bytes to header
//   - flag1 bit0 is used to indicate if the encoded stream uses VECSXP 
//     references
//   - flag2 is unused.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define HEADER_LEN 4

#define FLAG_VECSXP_REF 0x01


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Resize the data capacity of a 'raw_buffer_t'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void expand_raw_buffer(raw_buffer_t *buffer, size_t len) {
  
  buffer->capacity = len;
  buffer->data = realloc(buffer->data, buffer->capacity);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Callback used by 'write_sexp()'
// @param user_data a void* to whatever the user passed in as the first 
//        argument of 'create_serialize_ctx()'
// @param buf the new data output by the serializer
// @param len the number of bytes in *buf
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_raw_buffer(void *user_data, void *buf, size_t len) {
  raw_buffer_t *buffer = (raw_buffer_t *)user_data;
  
  if (buffer->pos + len >= buffer->capacity) {
    expand_raw_buffer(buffer, (buffer->pos + len) * 2);
  }
  
  memcpy(buffer->data + buffer->pos, buf, len);
  buffer->pos += len;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Callback used by 'read_sexp()'
// @param buffer a void* to whatever the user passed in as the first 
//        argument of 'create_serialize_ctx()'
// @param buf the new data to feed the unserializer. Note: there data may 
//        by modified in-place during unserialization. Use a copy of your actual
//        data
// @param len the number of bytes wanted by the unserializer
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_raw_buffer(void *user_data, void *buf, size_t len) {
  raw_buffer_t *buffer = (raw_buffer_t *)user_data;
  
  if (buffer->pos + len > buffer->capacity) {
    Rf_error("read_raw_buffer(): Read out-of-bounds at pos: %li. (%li + %li) >= %li", (long)buffer->pos, 
             (long)buffer->pos, (long)len, (long)buffer->capacity);
  }
  
  memcpy(buf, buffer->data + buffer->pos, len);
  buffer->pos += len;
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write an object to a raw vector
// - parse any options from the user
// - create a raw_buffer_t buffer
// - create a serialization context, specifying:
//      - buffer to passed to the callback
//      - the actual callbak function
//      - the options
// - serialize the object by calling 'write_sexp()'
// - convert the data in 'raw_buffer_t *buffer' to an R raw vector
// - insert the 2-byte header
// - return raw vector to R
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP write_zap_(SEXP obj_, SEXP dst_, SEXP opts_) {
  int nprotect = 0;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Parse any options from the user
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  opts_t *opts = parse_options(opts_);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize a 'raw_buffer_t *buffer'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  raw_buffer_t *buffer = malloc(sizeof(raw_buffer_t));
  if (buffer == NULL) Rf_error("buffer malloc failed");
  buffer->pos = 0;
  buffer->capacity = 512 * 1024;
  buffer->data = malloc(buffer->capacity);
  if (buffer->data == NULL) {
    free(buffer);
    Rf_error("buffer->data malloc failed");
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - create a serialization context, specifying:
  //      - buffer to passed to the callback
  //      - the actual callbak function
  //      - the options
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ctx_t *ctx = create_serialize_ctx(buffer,
                                    write_raw_buffer,
                                    opts);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - serialize the object by calling 'write_sexp()'
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  write_sexp(ctx, obj_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - convert the data in 'raw_buffer_t *buffer' to an R raw vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = R_NilValue;
  res_ = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)(buffer->pos + HEADER_LEN))); nprotect++;
  memcpy(RAW(res_) + HEADER_LEN, buffer->data, buffer->pos);
  
  //---------------------------------------------------------------------------
  // - insert the 2-byte header at the start of the raw data
  //---------------------------------------------------------------------------
  uint8_t *p = (uint8_t *)RAW(res_);
  p[0] = 'Z' | 0x80;
  p[1] = ZAP_VERSION;  
  p[2] = opts->vec_transform == ZAP_VEC_REF;  // lowest bit indicates if list references are used
  p[3] = 0x00; // Unused
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - tidy memory
  // - return raw vector to R
  // - If (verbosity & 64) then return the tally structure, not the data!
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (ctx->opts->verbosity & 64) {
    res_ = PROTECT(VECTOR_ELT(ctx->cache, ZAP_CACHE_TALLY)); nprotect++;
    set_df_attributes(res_);
  }
  
  ctx_destroy(ctx);
  free(buffer->data);
  free(buffer);
  free(opts);
  UNPROTECT(nprotect);
  return res_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unserialize an object from a raw vector
//
// - parse any user options
// - Check header is valid.
// - Create 'raw_buffer_t *' which points to the data in the given raw vector
// - Create a context for the unserialization
//    - user_data which will get passed to the callback
//    - the callback used by 'read_sexp()' to fetch bytes
//    - user options
// - Unserialize data to an R object
// - Tidy memory and return unserialized object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_zap_(SEXP src_, SEXP opts_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - parse any user options
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  opts_t *opts = parse_options(opts_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - Check header is valid.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint8_t *p = (uint8_t *)RAW(src_);
  if (p[0] != ('Z' | 0x80)) {
    Rf_error("unzap_raw(): Does not appear to be 'zap', serialized data");
  }
  if (p[1] != ZAP_VERSION) {
    Rf_warning("read_zap_(): Version numbers to not match. Expecting %i, Found %i\nAttempting to continue ... ", 
             ZAP_VERSION, p[1]);
  }
  if (p[2] & FLAG_VECSXP_REF) {
    opts->vec_transform = ZAP_VEC_REF;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - Create 'raw_buffer_t *' which points to the data in the given raw vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  raw_buffer_t *buffer = malloc(sizeof(raw_buffer_t));
  if (buffer == NULL) Rf_error("unzap_raw(): buffer malloc failed");
  
  buffer->pos = 0;
  buffer->capacity = (size_t)Rf_length(src_) - HEADER_LEN;
  buffer->data = RAW(src_) + HEADER_LEN;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - Create a context for the unserialization
  //    - user_data which will get passed to the callback
  //    - the callback used by 'read_sexp()' to fetch bytes
  //    - user options
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ctx_t *ctx = create_unserialize_ctx(buffer,
                                      read_raw_buffer,
                                      opts);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - Unserialize data to an R object
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP res_ = PROTECT(read_sexp(ctx));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // - Tidy memory and return unserialized object
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ctx_destroy(ctx);
  free(buffer);
  free(opts);
  UNPROTECT(1);
  return res_;
}

