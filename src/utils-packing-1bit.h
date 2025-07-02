
size_t pack_na_str(ctx_t *ctx, int BUF_IDX, SEXP x_);
void unpack_na_str(ctx_t *ctx, int BUF_IDX, SEXP x_, size_t len);

size_t pack_na_int(ctx_t *ctx, int BUF_IDX, SEXP x_);
void unpack_na_int(ctx_t *ctx, int BUF_IDX, SEXP x_, size_t len);

size_t pack_lgl(ctx_t *ctx, int BUF_IDX, SEXP x_);
void unpack_lgl(ctx_t *ctx, int BUF_IDX, SEXP x_, size_t len);
