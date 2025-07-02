
test_that("CPLXSXP works", {

  N <- 10000
  x <- complex(N)
  enc <- zap_write(x, NULL)
  round(length(enc) / (N * 8 * 2), 3)
  res <- zap_read(enc)
  expect_identical(res, x)
  
  
  set.seed(1)
  N <- 10000
  x <- complex(real = runif(N), imaginary = runif(N))
  enc <- zap_write(x, NULL)
  round(length(enc) / (N * 8 * 2), 3)
  res <- zap_read(enc)
  expect_identical(res, x)
  
  
  set.seed(1)
  N <- 10000
  x <- complex(real = round(runif(N), 3), imaginary = round(runif(N), 1))
  enc <- zap_write(x, NULL)
  round(length(enc) / (N * 8 * 2), 3)
  res <- zap_read(enc)
  expect_identical(res, x)
  
  
})
