

void zigzag_delta_encode_ptr_ptr(void *src, void *dst, size_t n_ints);
void zigzag_delta_encode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints);
void zigzag_delta_encode_ptr_buf(ctx_t *ctx, void *src, int dst_buf, size_t n_ints);

void zigzag_delta_decode_ptr_ptr(void *src, void *dst, size_t n_ints);
void zigzag_delta_decode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints);
void zigzag_delta_decode_buf_ptr(ctx_t *ctx, int src_buf, void *dst, size_t n_ints);





void zigzag_encode_ptr_ptr(void *src, void *dst, size_t n_ints);
void zigzag_encode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints);
void zigzag_encode_ptr_buf(ctx_t *ctx, void *src, int dst_buf, size_t n_ints);

void zigzag_decode_ptr_ptr(void *src, void *dst, size_t n_ints);
void zigzag_decode_buf_buf(ctx_t *ctx, int src_buf, int dst_buf, size_t n_ints);
void zigzag_decode_buf_ptr(ctx_t *ctx, int src_buf, void *dst, size_t n_ints);
