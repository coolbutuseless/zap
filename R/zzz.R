

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# If 'zstd' works with memCOmpress, then set this as the default.
# otherwise use 'gzip'
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.onLoad <- function(libname, pkgname){
  tryCatch({
    memCompress(raw(1), 'zstd')
    Sys.setenv(zap_compress_default = "zstd")
  },
  error = function(e) {
    Sys.setenv(zap_compress_default = "gzip")
  }
  )
}

