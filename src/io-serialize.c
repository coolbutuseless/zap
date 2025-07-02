
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "io-ctx.h"
#include "io-serialize.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###                         #    
//  #   #                        #    
//  #       ###   #   #  # ##   ####  
//  #      #   #  #   #  ##  #   #    
//  #      #   #  #   #  #   #   #    
//  #   #  #   #  #  ##  #   #   #  # 
//   ###    ###    ## #  #   #    ##
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Callbacks required by R's serialization framework for handling writing of:
//   - a single byte   NOTE: This function is unused for binary serialization.
//   - multiple bytes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void count_byte(R_outpstream_t stream, int c) {
  Rf_error("count_byte(): This function is never called for binary serialization");
}

void count_bytes(R_outpstream_t stream, void *src, int length) {
  size_t *count = (size_t *)stream->data;
  *count += (size_t)length;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object, but only count the bytes.  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t calc_serialized_size(SEXP robj) {
  
  // Create the user data passed into the serialization callbacks
  size_t count = 0;
  
  // Create the output stream structure
  struct R_outpstream_st output_stream;
  
  // Initialise the output stream structure
  R_InitOutPStream(
    &output_stream,            // The stream object which wraps everything
    (R_pstream_data_t) &count, // user data that persists within the process
    R_pstream_binary_format,   // Store as binary
    3,                         // Version = 3 for R >3.5.0 See `?base::serialize`
    count_byte,                // Function to write single byte to buffer
    count_bytes,               // Function for writing multiple bytes to buffer
    NULL,                      // Func for special handling of reference data.
    R_NilValue                 // Data related to reference data handling
  );
  
  // Serialize the object into the output_stream
  R_Serialize(robj, &output_stream);
  
  return count;
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###                   #            ##      #                  
//  #   #                                #                         
//  #       ###   # ##    ##     ###     #     ##    #####   ###   
//   ###   #   #  ##  #    #        #    #      #       #   #   #  
//      #  #####  #        #     ####    #      #      #    #####  
//  #   #  #      #        #    #   #    #      #     #     #      
//   ###    ###   #       ###    ####   ###    ###   #####   ###   
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The user data passed to serialization callbacks
// This tracks the state 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  size_t length;       // Total length of the buffer
  size_t pos;          // The current position in which to write the next data
  unsigned char *data; // The actual serialized data
} static_buffer_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Callbacks required by R's serialization framework for handling writing of:
//   - a single byte   NOTE: This function is unused for binary serialization.
//   - multiple bytes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_to_static_buffer(R_outpstream_t stream, int c) {
  Rf_error("write_byte_to_static_buffer(): This function is never used in binary serialization");
}


void write_bytes_to_static_buffer(R_outpstream_t stream, void *src, int length) {
  
  // Unpack the user data
  static_buffer_t *buf = (static_buffer_t *)stream->data;
  
  // Sanity check we're not writing out-of-bounds
  if (buf->pos + (size_t)length > buf->length) {
    Rf_error("write_bytes_to_static_buffer(): overflow! %li + %i > %li\n", 
             (long)buf->pos, length, (long)buf->length);
  }
  
  // Copy the data and advance the 'pos' pointer
  memcpy(buf->data + buf->pos, src, length);
  buf->pos += (size_t)length;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read byte/bytes from the serialized stream
// Single byte reading is never user with binary serialization
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int read_byte_from_static_buffer(R_inpstream_t stream) {
  Rf_error("read_byte_from_static_buffer(): This function is never used in binary serialization");
}


void read_bytes_from_static_buffer(R_inpstream_t stream, void *dst, int length) {
  
  // Unpack the user data
  static_buffer_t *buf = (static_buffer_t *)stream->data;
  
  // Sanity check we're not reading out-of-bounts
  if (buf->pos + (size_t)length > buf->length) {
    Rf_error("read_bytes_from_static_buffer(): overflow");
  }
  
  // copy the data and advance the 'pos' pointer
  memcpy(dst, buf->data + buf->pos, length);
  buf->pos += (size_t)length;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unserialize a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP runserialize(ctx_t *ctx, int buf_idx, size_t len) {

  // In C, we want the data as a 'void *'
  void *vec = (void *)ctx->buf[buf_idx];

  // Create a buffer object which points to the raw data
  static_buffer_t buf;
  buf.length = len;
  buf.pos    = 0;
  buf.data   = vec;

  // Treat the data buffer as an input stream
  struct R_inpstream_st input_stream;

  R_InitInPStream(
    &input_stream,                 // Stream object wrapping data buffer
    (R_pstream_data_t) &buf,        // user data which persists during the serialization
    R_pstream_any_format,          // Unpack all serialized types
    read_byte_from_static_buffer,  // Function to read single byte from buffer
    read_bytes_from_static_buffer, // Function for reading multiple bytes from buffer
    NULL,                          // Func for special handling of reference data.
    NULL                           // Data related to reference data handling
  );

  // Unserialize the input_stream into an R object
  return R_Unserialize(&input_stream);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object to a C uint8_t vector
// It is the callers responsibility to free this memory
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
size_t rserialize(ctx_t *ctx, int buf_idx, SEXP robj) {

  // Figure out how much memory is needed
  size_t len = calc_serialized_size(robj);
  prepare_buf(ctx, buf_idx, len);
  
  // Create a buffer object which points to the raw data
  static_buffer_t buf;
  buf.length = len;
  buf.pos    = 0;
  buf.data   = ctx->buf[buf_idx];

  // Create the output stream structure
  struct R_outpstream_st output_stream;

  // Initialise the output stream structure
  R_InitOutPStream(
    &output_stream,               // The stream object which wraps everything
    (R_pstream_data_t) &buf,      // user data which persists during the serialization
    R_pstream_binary_format,      // Serialize as binary
    3,                            // Version = 3 for R >3.5.0 See `?base::serialize`
    write_byte_to_static_buffer,  // Function to write single byte to buffer
    write_bytes_to_static_buffer, // Function for writing multiple bytes to buffer
    NULL,                         // Func for special handling of reference data.
    R_NilValue                    // Data related to reference data handling
  );

  // Serialize the object into the output_stream
  R_Serialize(robj, &output_stream);

  // Tidy and return
  return len;
}




#define BUF_SERIALIZED 0

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write an object by first serializing with R's builtin mechanism
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_rserialize(ctx_t *ctx, SEXP x_) {
  
  size_t serial_len = rserialize(ctx, BUF_SERIALIZED, x_);
  
  write_uint8(ctx, RSERIALSXP);
  write_buf(ctx, BUF_SERIALIZED, serial_len);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read an object by unserializing a raw vector using R's builtin mechanism
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP read_rserialize(ctx_t *ctx) {
  
  size_t serial_len = read_buf(ctx, BUF_SERIALIZED);

  return runserialize(ctx, BUF_SERIALIZED, serial_len);
}





