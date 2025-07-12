void set_df_attributes(SEXP df_);
SEXP create_named_list(int n, ...);
SEXP get_df_col(SEXP df_, const char *str);

void objdf_add_row(SEXP df_, int idx, int type, int loc, int depth);
void df_grow(SEXP df_);
void df_truncate(SEXP df_, int nrows);

