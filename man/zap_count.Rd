% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/zap.R
\name{zap_count}
\alias{zap_count}
\title{Count the uncompressed bytes to serialize this object}
\usage{
zap_count(x, opts = list(), ...)
}
\arguments{
\item{x}{R object}

\item{opts}{Named list of options.   See \code{\link{zap_opts}()}}

\item{...}{other named options to be included in \code{opts}. See
\code{\link{zap_opts}()} for list of valid options.}
}
\value{
Integer
}
\description{
Count the uncompressed bytes to serialize this object
}
\examples{
zap_count(mtcars)
length(zap_write(mtcars))
}
