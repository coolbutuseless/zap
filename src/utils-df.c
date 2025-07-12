
#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include "utils-df.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @param df_ named list object which is to be promoted to data.frame
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void set_df_attributes(SEXP df_) {
  int nprotect = 0;
  
  if (!Rf_isNewList(df_)) {
    Rf_error("set_df_attributes(): only accepts 'lists' as input");
  }
  
  int len = Rf_length(VECTOR_ELT(df_, 0));
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set row.names
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rownames = PROTECT(Rf_allocVector(INTSXP, 2)); nprotect++;
  SET_INTEGER_ELT(rownames, 0, NA_INTEGER);
  SET_INTEGER_ELT(rownames, 1, -len);
  Rf_setAttrib(df_, R_RowNamesSymbol, rownames);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set as tibble
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP classnames = PROTECT(Rf_allocVector(STRSXP, 3)); nprotect++;
  SET_STRING_ELT(classnames, 0, Rf_mkChar("tbl_df"));
  SET_STRING_ELT(classnames, 1, Rf_mkChar("tbl"));
  SET_STRING_ELT(classnames, 2, Rf_mkChar("data.frame"));
  SET_CLASS(df_, classnames);
  
  UNPROTECT(nprotect);
} 



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create a named list  
//   - fill get turned into data.frame before return with 'set_df_attributes'
//     once we know the number of rows
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP create_named_list(int n, ...) {
  
  va_list args;
  va_start(args, n);
  
  int nprotect = 0;
  SEXP res_ = PROTECT(Rf_allocVector(VECSXP, n)); nprotect++;
  SEXP nms_ = PROTECT(Rf_allocVector(STRSXP, n)); nprotect++;
  Rf_setAttrib(res_, R_NamesSymbol, nms_);
  
  for (int i = 0; i < n; i++) {
    
    const char *nm = va_arg(args, const char *);
    SEXP val_ = va_arg(args, SEXP);
    
    SET_STRING_ELT(nms_, i, Rf_mkChar(nm));
    SET_VECTOR_ELT(res_, i, val_);
  }
  
  
  va_end(args);
  UNPROTECT(nprotect);
  return res_;
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Grow a data.frame
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void df_grow(SEXP df_) {
  SEXP col0 = VECTOR_ELT(df_, 0);
  int current_nrows = Rf_length(col0);
  int new_nrows = 2 * current_nrows;
  
  for (int i = 0; i < Rf_length(df_); i++) {
    SEXP col_ = VECTOR_ELT(df_, i);
    SEXP newcol_ = PROTECT(Rf_lengthgets(col_, (R_len_t)new_nrows));
    SET_VECTOR_ELT(df_, i, newcol_);
    UNPROTECT(1);
  }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Data.frames are overallocated during the accumulation of device calls.
// These data.frames are truncated back to their final size before returning
// them to R
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void df_truncate(SEXP df_, int nrows) {
  SEXP col0 = VECTOR_ELT(df_, 0);
  int current_nrows = Rf_length(col0);
  if (nrows == current_nrows) {
    return;
  } else if (nrows > current_nrows) {
    Rf_error("df_truncate issue: %i > %i", nrows, current_nrows);
  }
  
  for (int i = 0; i < Rf_length(df_); i++) {
    SEXP col_ = VECTOR_ELT(df_, i);
    SEXP newcol_ = PROTECT(Rf_lengthgets(col_, (R_len_t)nrows));
    SET_VECTOR_ELT(df_, i, newcol_);
    UNPROTECT(1);
  }
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Add a row to the specific "objs tally" data.frame
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void objdf_add_row(SEXP df_, int idx, int depth, int type, int start, 
                   int end, int altrep, int rserialize, int has_attrs) {
  
  SEXP col_ = VECTOR_ELT(df_, 0);
  INTEGER(col_)[idx] = depth;
  
  col_ = VECTOR_ELT(df_, 1);
  INTEGER(col_)[idx] = type;

  col_ = VECTOR_ELT(df_, 2);
  INTEGER(col_)[idx] = start + 4L + 1L;  // header length + convert to R idx

  col_ = VECTOR_ELT(df_, 3);
  INTEGER(col_)[idx] = end + 4 + 1L - 1L;  // header length + convert to R idx

  col_ = VECTOR_ELT(df_, 4);
  INTEGER(col_)[idx] = altrep;
  
  col_ = VECTOR_ELT(df_, 5);
  INTEGER(col_)[idx] = rserialize;
  
  col_ = VECTOR_ELT(df_, 6);
  INTEGER(col_)[idx] = has_attrs;
}












