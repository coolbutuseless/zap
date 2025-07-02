
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Work-a-like replacement for built-in 'modifyList'
#' 
#' @param old,new lists
#' @return updated list
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
modify_list <- function(old, new) {
  for (nm in names(new)) old[[nm]] <- new[[nm]]
  old
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Idea borrowed from \code{ggplot2::theme()}
#'
#' Use of this function negates having to define default values for all the
#' arguments in the parent function
#'
#' @param ... all arguments from call to parent function
#'
#' @return list with only arguments used in the call
#' @importFrom utils modifyList
#' @noRd
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
find_args <- function (...) {
  env <- parent.frame()
  args <- names(formals(sys.function(sys.parent(1))))
  vals <- mget(args, envir = env)
  vals <- vals[!vapply(vals, function(x) {identical(x, quote(expr=))}, logical(1))]
  args <- modify_list(vals, list(..., ... = NULL))
  
  args
}

