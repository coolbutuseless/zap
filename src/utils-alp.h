
typedef struct {
  int e;
  int f;
  int score;
  int gap;
  size_t npatch;
  size_t ntest;
} alp_params_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Probe the given double vector by
//  -  sampling N equi-spaced values
//  -  finding the best params for this sampling
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alp_params_t alp_probe(double *x, size_t len, size_t N);

alp_params_t alp_probe_full(double *x, size_t len);

void alp_encode(double *src, int64_t *dst, size_t len, int e, int f, double *patch, 
                uint32_t *patch_idx, uint32_t *npatch);

void alp_decode(int64_t *src, double *dst, size_t len, int e, int f, double *patch, 
                uint32_t *patch_idx, uint32_t npatch);

