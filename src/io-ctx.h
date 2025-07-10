
// Hashmap
#include "mph.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Version of set of internal transformations
// Anytime a transformation changes or is added, bump this version by 1
// This number is written as the second byte of an uncompressed zap stream
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ZAP_VERSION 1


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Number of internal buffers to pre-allocate
// These are the working buffers used during transformation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define CTX_NBUFS 5


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Two special verbosity modes
//    TALLY outputs a tally at the end of the serialization
//    TRE   outputs an indented tree of all SEXPs as they are written
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ZAP_VERB_TALLY 16
#define ZAP_VERB_TREE  32


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Transformation methods
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ZAP_LGL_RAW        0  // Uncompressed
#define ZAP_LGL_PACKED     1  // Packed into 2 bitstreams

#define ZAP_INT_RAW        0  // Uncompressed
#define ZAP_INT_ZZSHUF     1  // ZigZag + Delta + Shuffle
#define ZAP_INT_DELTAFRAME 2  // delta frame-of-reference

#define ZAP_FCT_RAW        0  // Uncompressed
#define ZAP_FCT_PACKED     1  // Packed into minimal nbits per element

#define ZAP_DBL_RAW        0  // Uncompressed
#define ZAP_DBL_SHUF       1  // shuffle bytes
#define ZAP_DBL_SHUF_DELTA 2  // delta + shuffle bytes
#define ZAP_DBL_ALP        3  // ALP

#define ZAP_STR_RAW        0  // Uncompressed
#define ZAP_STR_MEGA       1  // Mega string

#define ZAP_VEC_RAW        0
#define ZAP_VEC_REF        1

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Cache contents
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define ZAP_CACHE_ENVSXP  0
#define ZAP_CACHE_VECSXP  1


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// All the user specified options
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  
  int verbosity;
  
  int dbl_fallback;
  
  int lgl_transform;
  int int_transform;
  int fct_transform;
  int dbl_transform;
  int str_transform;
  int vec_transform;
  
  int lgl_threshold;
  int int_threshold;
  int fct_threshold;
  int dbl_threshold;
  int str_threshold;
} opts_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// zap transformation context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  
  // The temporary storage buffers for transformation
  uint8_t *buf[CTX_NBUFS];
  size_t bufsize[CTX_NBUFS];
  
  // Storage for environments seen during serialization so that references
  // can be used
  SEXP cache;
  
  R_xlen_t Nenv;
  mph_t *envsxp_hashmap;

  R_xlen_t Nvecsxp;
  mph_t *vecsxp_hashmap;
  
  // user supplied data passed to callbacks
  // the two primary callbacks used to read/write data
  void *user_data; 
  void    (*write)     (void *user_data, void *buf, size_t len);
  void    (*read)      (void *user_data, void *buf, size_t len);
  
  // Storage and tracking for verbose output
  int depth;            // tracking depth for tree printing
  int tally_sexp  [32]; // Vanilla SEXP objects
  int tally_altrep[32]; // ALTREP objects
  int tally_serial[32]; // SEXP types which were serialized by the internal serialization
  
  // User options
  opts_t *opts;
} ctx_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Creat a serialization context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ctx_t *create_serialize_ctx(void *user_data, 
                            void    (*write)(void *user_data, void *buf, size_t len),
                            opts_t *opts);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create an Unserialization context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ctx_t *create_unserialize_ctx(void *user_data,
                              void    (*read)(void *user_data, void *buf, size_t len),
                              opts_t *opts);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Destroy the context and all allocated memory. Release any cached R objects
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ctx_destroy(ctx_t *ctx);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// New SEXPs to take care of special cases.
// "spare" values below 32
//    - 11,12 once used but no longer
//    - 26-31
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define RSERIALSXP 31  // Use R's serialization 
#define FACTORSXP  30  // This is a factor


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse list of user options to C struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
opts_t *parse_options(SEXP opts_);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Forward declaration of generic IO
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








