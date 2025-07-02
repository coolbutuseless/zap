
size_t pack_nbits_ptr_ptr(ctx_t *ctx, uint32_t *src, void *dst  , size_t N_orig_ints, size_t nbits);
size_t pack_nbits_ptr_buf(ctx_t *ctx, uint32_t *src, int BUF_DST, size_t N_orig_ints, size_t nbits);

void unpack_nbits_ptr_ptr(ctx_t *ctx, void *src  , uint32_t *dst, size_t N_orig_ints, size_t nbits);
void unpack_nbits_buf_ptr(ctx_t *ctx, int BUF_SRC, uint32_t *dst, size_t N_orig_ints, size_t nbits);
