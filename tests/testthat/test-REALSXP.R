

test_that("multiplication works", {

  N <- 10000
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # All zeros
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  x <- double(N)
  enc <- zap_write(x, NULL)
  round(length(enc) / (N * 8), 3)
  res <- zap_read(enc)
  expect_identical(res, x)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Purely random
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  set.seed(1)
  x <- runif(N)
  enc <- zap_write(x, NULL)
  round(length(enc) / (N * 8), 3)
  res <- zap_read(enc)
  expect_identical(res, x)
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # Purely random
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  set.seed(1)
  x <- round(runif(N), 3)
  enc <- zap_write(x, NULL)
  round(length(enc) / (N * 8), 3)
  res <- zap_read(enc)
  expect_identical(res, x)
  
  
  
  
  
})
