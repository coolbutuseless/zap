

test_that("INTSXP works", {
  
  N <- 1000
  
  
  for (range in 10^(1:15)) {
    
    vec <- sample(seq(-range, range), size = N, replace = TRUE)
    enc <- zap_write(vec, NULL)
    dec <- zap_read(enc)
    
    expect_identical(dec, vec)
  }
  
  
  
  
  for (range in 10^(1:15)) {
    
    vec <- sample(seq(-range, range), size = N, replace = TRUE)
    enc <- zap_write(vec, NULL, int = 'zzshuf')
    dec <- zap_read(enc)
    
    expect_identical(dec, vec)
  }
  
  
  
  
  for (range in 10^(1:15)) {
    
    vec <- sample(seq(-range, range), size = N, replace = TRUE)
    enc <- zap_write(vec, NULL, int = 'deltaframe')
    dec <- zap_read(enc)
    
    expect_identical(dec, vec)
  }
  
  
  set.seed(1)
  vec <- sample(10)
  # vec[3] <- NA_integer_
  enc <- zap_write(vec, NULL, int = 'deltaframe')
  dec <- zap_read(enc)
  vec
  dec
  expect_identical(dec, vec)  
  
  
  set.seed(1)
  vec <- sample(c(1L, 2L, 3L, NA_integer_), 32, T)
  enc <- zap_write(vec, NULL, int = 'deltaframe')
  dec <- zap_read(enc)
  vec
  dec
  expect_identical(is.na(dec), is.na(vec))  
  expect_identical(dec, vec)  
  
  
  
})



test_that("All NA INTSXP works", {
  
  vec <- rep(NA_integer_, 10000)
  enc <- zap_write(vec, NULL)
  dec <- zap_read(enc)
  expect_identical(is.na(dec), is.na(vec))  
  expect_identical(dec, vec)  
  
  
  
})








