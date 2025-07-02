
size_t  deltaframe_encode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints, int32_t *ref, int32_t *delta_offset, size_t *nbits); 
void    deltaframe_decode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints, int32_t  ref, int32_t  delta_offset, size_t nbits);

size_t  deltaframe_encode_ptr_buf(ctx_t *ctx, void *src  , int dst_buf, size_t n_ints, int32_t *ref, int32_t *delta_offset, size_t *nbits); 
void    deltaframe_decode_buf_ptr(ctx_t *ctx, int src_buf, void *dst  , size_t n_ints, int32_t  ref, int32_t  delta_offset, size_t nbits);
