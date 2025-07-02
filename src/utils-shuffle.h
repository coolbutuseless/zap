
void   shuffle8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls); 
void   shuffle8_ptr_buf(ctx_t *ctx, void *src  , int dst_buf, size_t n_dbls); 
void unshuffle8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls);
void unshuffle8_buf_ptr(ctx_t *ctx, int src_buf, void *dst  , size_t n_dbls);

void   shuffle_delta8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls); 
void   shuffle_delta8_ptr_buf(ctx_t *ctx, void *src  , int dst_buf, size_t n_dbls); 
void unshuffle_delta8_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_dbls);
void unshuffle_delta8_buf_ptr(ctx_t *ctx, int src_buf, void *dst  , size_t n_dbls);

void   shuffle_delta4_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints); 
void   shuffle_delta4_ptr_buf(ctx_t *ctx, void *src  , int dst_buf, size_t n_ints); 
void unshuffle_delta4_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints);
void unshuffle_delta4_buf_ptr(ctx_t *ctx, int src_buf, void *dst  , size_t n_ints);
