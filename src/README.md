
# Code layout

The code is roughly divided into 3 groups

* `io` code for reading/writing SEXP
    * one file for each SEXP (or faux-SEXP e.g. factors)
    * IO files for SEXPs may also include transformation code, but 
      usually only if it is specific for that SEXP
* `utils` reusable code for data transformations. Possibly used across 
  multiple SEXPs.
* `zap` the interface to the R code. 


## `utils` code

These are mostly re-usable functions for applying data transformations - so there
are `encode` and `decode` functions.

The enocde/decode functions come in four possible variants

* `ptr_ptr` (from `ptr` to `ptr`). For transforming data from one C pointer to another.  This is 
  rarely called directly as the caller has to ensure enough memory is allocated.
* `ptr_buf`  (from `ptr` to `buf`). For encoding data with the source
  data direct from the SEXP (the input `ptr`) and write to one of the 
  pre-allocated `ctx->buf` buffers.
* `buf_ptr`  (from `buf` to `ptr`). For decoding data, 
  write the destination
  data direct to the SEXP (the output `ptr`) while reading from one of the 
  pre-allocated `ctx->buf` buffers.
* `buf_buf`  (from `buf` to `buf`).  Transform data between 
  pre-allocated `ctx->buf` buffers.
  
The `buf` variants ensure that the `ctx->buf` buffer being
used has allocated enough memory for the transformation.


# Writing a new transformation

If you wanted to write a new transformation:

* Create a `utils-mytransform.c` and `.h`
* Write core functions:
    * `size_t mytransform_encode_ptr_ptr(ctx_t *ctx, (void *)src, (void *dst), size_t len)`
    * `void mytransform_decode_ptr_ptr(ctx_t *ctx, (void *)src, (void *dst), size_t len)`
    * This function should assume that all pointers are already appropriately sized.
    * The `encode` variant should return the number of bytes written to `dst`
* Write as many of the variants as you need:
    * `buf_ptr`
    * `ptr_buf`
    * `buf_buf`
    * `buf_buf`

  
  
  
  
  