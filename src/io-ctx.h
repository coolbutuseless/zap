


#define ZAP_VERSION 1

#define CTX_READ  0
#define CTX_WRITE 1

#define CTX_NBUFS 5

#define ZAP_COMP_NONE    0
#define ZAP_COMP_ZSTD    1


#define ZAP_VERB_TALLY 16
#define ZAP_VERB_TREE  32

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Atomic methods
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ZAP_LGL_RAW    0
#define ZAP_LGL_PACKED 1


#define ZAP_INT_RAW        0
#define ZAP_INT_ZZSHUF     1
#define ZAP_INT_DELTAFRAME 2


#define ZAP_FCT_RAW    0
#define ZAP_FCT_PACKED 1


#define ZAP_DBL_RAW        0
#define ZAP_DBL_SHUF       1
#define ZAP_DBL_SHUF_DELTA 2
#define ZAP_DBL_ALP        3


#define ZAP_STR_RAW  0
#define ZAP_STR_MEGA 1

typedef struct {
  
  int verbosity;
  int compression;
  
  int dbl_fallback;
  int zstd_level;
  
  int lgl_transform;
  int int_transform;
  int fct_transform;
  int dbl_transform;
  int str_transform;
  
  int lgl_threshold;
  int int_threshold;
  int fct_threshold;
  int dbl_threshold;
  int str_threshold;
  
  
} opts_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Compression/decompression context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int mode;
  void *user_data;
  
  uint8_t *buf[CTX_NBUFS];
  size_t bufsize[CTX_NBUFS];
  
  R_xlen_t Nenv;
  SEXP env_list;
  
  void    (*write)     (void *user_data, void *buf, size_t len);
  void    (*read)      (void *user_data, void *buf, size_t len);
  
  int depth;            // tracking detpth for tree printing
  int tally_sexp  [32]; // Vanilla SEXP objects
  int tally_altrep[32]; // ALTREP objects
  int tally_serial[32]; // SEXP types which were serialized by the internal serialization
  
  opts_t *opts;
} ctx_t;


ctx_t *create_serialize_ctx(void *user_data, 
                            void    (*write)(void *user_data, void *buf, size_t len),
                            opts_t *opts);

ctx_t *create_unserialize_ctx(void *user_data,
                              void    (*read)(void *user_data, void *buf, size_t len),
                              opts_t *opts);

void ctx_destroy(ctx_t *ctx);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// New SEXPs to take care of special cases.
// "spare" values below 32
//    - 11,12 once used but no longer
//    - 26-31
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define RSERIALSXP 31
#define FACTORSXP  30

opts_t *parse_options(SEXP opts_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void prepare_buf(ctx_t *ctx, int idx, size_t len);
void realloc_buf(ctx_t *ctx, int idx, size_t len);

void write_buf(ctx_t *ctx, int buf_idx, size_t len);
size_t read_buf(ctx_t *ctx, int buf_idx);

void write_ptr(ctx_t *ctx, void *ptr, size_t len);
size_t read_ptr(ctx_t *ctx, void *ptr);

void write_int32(ctx_t *ctx, int32_t val);
int32_t read_int32(ctx_t *ctx);

void write_uint8(ctx_t *ctx, uint8_t val);
uint8_t read_uint8(ctx_t *ctx);

// For read_len()/write_len()
#include "utils-varint.h"

void dump_tally(ctx_t *ctx);








