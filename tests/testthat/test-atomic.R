

test_that("atomic i/o works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Create some random atomics of each type
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  set.seed(1)
  refs <- list(
    as.raw(sample(100)),     # raw
    sample(100) > 50,        # lgl
    sample(100),             # int
    as.double(sample(100)),  # dbl
    as.complex(sample(100)), # cmplx
    as.factor(sample(letters, 100, T)), # factor
    sample(c("a", "ab", "za", "mmmmmmmmmmmmm"), 100, T) # words
  )
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # for each item: write then read  then assert identical
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (ref in refs) {
    enc <- zap_write(ref)
    res <- zap_read(enc)
    expect_identical(res, ref)
  }
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # For the list as a whole, write it and read it back
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  enc <- zap_write(refs)
  res <- zap_read(enc)
  expect_identical(res, refs)
})




test_that("atomic NAs work", {
  
  ref <- list(NA, NA_integer_, NA_real_, NA_complex_, NaN, Inf, -Inf) 
  enc <- zap_write(ref)
  res <- zap_read(enc)
  expect_identical(res, ref)
  
})


test_that("length-0 atomics work", {
  
  ref <- list(
    logical(0),
    integer(0),
    double(0),
    numeric(0),
    complex(0)
  ) 
  enc <- zap_write(ref)
  res <- zap_read(enc)
  expect_identical(res, ref)
  
})







test_that("basic attributes work", {
  ref <- 1:3
  names(ref) <- c("a", "ab", "abc")
  attr(ref, "hello") <- 1
  attr(ref, "bye") <- "done"
  enc <- zap_write(ref)
  result <- zap_read(enc)
  attributes(result)
  expect_identical(result, ref)
})



test_that("matrix works", {
  
  mat <- matrix(1:10, 2, 5)
  
  enc <- zap_write(mat)
  result <- zap_read(enc)
  attributes(result)
  expect_identical(result, mat)
})


