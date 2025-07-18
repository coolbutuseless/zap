

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Get version of internal transformation code.
#' 
#' This version number is the same as the version number present in the
#' header of the serialized data.
#' 
#' @return Integer
#' @examples
#' zap_version()
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zap_version <- function() {
  .Call(zap_version_)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Count the uncompressed bytes to serialize this object
#' 
#' @inheritParams zap_write
#' @return Integer
#' @examples
#' zap_count(mtcars)
#' length(zap_write(mtcars))
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zap_count <- function(x, opts = list(), ...) {
  opts <- modify_list(opts, list(...));
  .Call(zap_count_, x, opts)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Create options list for writing data
#' 
#' @param verbosity Verbosity level. Default: 0 (no text output).
#' \describe{
#'   \item{64}{Return a data.frame with information on each SEXP within the object. 
#'        \code{start} and \code{end} values are the position of the object within
#'        the \emph{uncompressed} stream}
#' }
#' @param transform Enable transformations? Default: TRUE.  Setting to 
#'        FALSE will disable all transformations.
#' @param lgl transformation method for logical vectors. Default: 'packed'
#' \describe{
#'   \item{\code{raw}}{Raw. No transformation}
#'   \item{\code{packed}}{Packed 2 bits per logical value}
#' }
#' @param int transformation method for integer vectors. Default: 'deltaframe'
#' \describe{
#'   \item{\code{raw}}{Raw. No transformation}
#'   \item{\code{zzshuf}}{Zig-zag encoding, delta and shuffle}
#'   \item{\code{deltaframe}}{Delta frame-of-reference coding}
#' }
#' @param fct transformation method for factors vectors. Default: 'packed'
#' \describe{
#'   \item{\code{raw}}{Raw. No transformation}
#'   \item{\code{packed}}{Packed minimal bits per level}
#' }
#' @param dbl transformation method for doubles (and complex) vectors. Default: 'alp'
#' \describe{
#'   \item{\code{raw}}{Raw. No transformation}
#'   \item{\code{shuffle}}{Byte shuffle}
#'   \item{\code{delta_shuffle}}{Byte shuffle with delta}
#'   \item{\code{alp}}{ALP, Adaptive Lossless Floating Point compression}
#' }
#' @param list transformation method for lists (and data.frames).  Default: 'raw'
#' \describe{
#'   \item{\code{raw}}{Raw. All lists written out in-full}
#'   \item{\code{reference}}{Cache lists and data.frames as they are seen, and 
#'         if seen again, write out a reference to the prior object rather than 
#'         writing out in-full.}
#' }
#' @param str transformation method for character vectors. Default: 'mega'
#' \describe{
#'   \item{\code{raw}}{Raw. No transformation}
#'   \item{\code{mega}}{Concatenate all strings.  Length implicitly encoded by null bytes in strings}
#' }
#' @param int_threshold,lgl_threshold,fct_threshold,dbl_threshold,str_threshold 
#'        Below this threshold, no transformation will be done. All default to
#'        0, meaning transformation is always attempted.
#' @param dbl_fallback if \code{dbl = 'alp'}, the data is not
#'        always conducive to this compression scheme and after probing the
#'        data the code can exit early and try a different method.  
#'        The \code{dbl_fallback} variable nominates the fallback method if ALP
#'        transformation is being attempted, but fails. The options are the
#'        same as for the \code{dbl} argument (excluding option \code{'alp'})
#' @param ... expert level options
#' @return named list
#' @examples
#' myopts <- zap_opts(dbl = 'shuffle')
#' zap_write(seq(1:1000) * 1.5, opts = myopts)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zap_opts <- function(transform, verbosity, 
                     lgl, int, fct, dbl, str, 
                     list,
                     lgl_threshold, int_threshold, fct_threshold, 
                     dbl_threshold, str_threshold, 
                     dbl_fallback, ...) {
  
  find_args(...)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize R object to raw vector or file
#' 
#' @param x R object
#' @param dst Serialization destination. Default: NULL means to return the raw vector.
#'        If a character string is given it is assumed to be the path
#'        to the output file.
#' @param compress compression type. Default: 'zstd' if available, otherwise 'gzip'.
#'        This is set in the 'zap_compress_default' environment variable after
#'        being detected during package start.
#'        Other valid values 'none', 
#'        'xz', 'bzip2'.  Compression is done using \code{memCompress()}
#' @param opts Named list of options.   See \code{\link{zap_opts}()}
#' @param ... other named options to be included in \code{opts}. See
#'        \code{\link{zap_opts}()} for list of valid options.
#' @return IF \code{dst} is NULL, then return a raw vector, otherwise data
#'         is written to file and nothing is returned.
#' @examples
#' raw_vec <- zap_write(head(mtcars))
#' head(raw_vec, 50)
#' length(raw_vec)
#' zap_read(raw_vec)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zap_write <- function(x, dst = NULL, compress = Sys.getenv('zap_compress_default'), opts = list(), ...) {
  opts <- modify_list(opts, list(...));
  res <- .Call(write_zap_, x, dst, opts)
  if (is.data.frame(res)) {
    # This is the data.frame of object countschar *sexp_nms[32] = {
   sexp_names <- c(
     "NILSXP"	    ,
    "SYMSXP"	    ,
    "LISTSXP"	    ,
    "CLOSXP"	    ,
    "ENVSXP"	    ,
    "PROMSXP"	    ,
    "LANGSXP"	    ,
    "SPECIALSXP"  ,
    "BUILTINSXP"  ,
    "CHARSXP"	    ,
    "LGLSXP"	    ,
    "unused"      ,
    "unused"      ,
    "INTSXP"	    ,
    "REALSXP"	    ,
    "CPLXSXP"	    ,
    "STRSXP"	    ,
    "DOTSXP"	    ,
    "ANYSXP"	    ,
    "VECSXP"	    ,
    "EXPRSXP"	    ,
    "BCODESXP"    ,
    "EXTPTRSXP"   ,
    "WEAKREFSXP" , 
    "RAWSXP"     , 
    "S4SXP"	     , 
    "unused"     ,
    "unused"     ,
    "unused"     ,
    "unused"     ,
    "unused"     ,
    "unused"     
    )
    res$type <- factor(res$type + 1, levels = 1:32, labels = sexp_names);
    return(res);
  }
  res <- memCompress(res, type = compress)
  if (is.character(dst)) {
    writeBin(res, dst)
    invisible()
  } else {
    res
  }
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Unserialize R object from raw vector or file
#' 
#' @inheritParams zap_write
#' @param src Serialization source - either a raw vector of filename.
#' @return Unserialized R object
#' @examples
#' raw_vec <- zap_write(head(mtcars))
#' head(raw_vec, 50)
#' length(raw_vec)
#' zap_read(raw_vec)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
zap_read <- function(src, opts = list(), ...) {
  if (is.character(src)) {
    # Treat as a filename
    src <- readBin(src, 'raw', n = file.size(src))
  }
  
  # Uncompress src - using auto detection of compression type
  # Note that auto-detection of type can produce warnings()
  suppressWarnings({
    src <- memDecompress(src)
  })
  
  .Call(read_zap_, src, modify_list(opts, list(...)))
}


