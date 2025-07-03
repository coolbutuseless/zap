


test_that("logical bit packing works", {
  
  set.seed(1)
  
  
  for (N in seq(0, 73)) {
    # print(N)
    vec <- sample(c(TRUE, FALSE), N, replace = TRUE)
    enc <- zap_write(vec)
    result <- zap_read(enc)
    expect_identical(result, vec)
  }
  
  
  for (N in seq(0, 73)) {
    # print(N)
    vec <- rep(T, N)
    enc <- zap_write(vec)
    result <- zap_read(enc)
    expect_identical(result, vec, label = paste("LGLSXP TRUE n =", N))
  }
  
  
  for (N in seq(0, 73)) {
    # print(N)
    vec <- rep(F, N)
    enc <- zap_write(vec)
    result <- zap_read(enc)
    expect_identical(result, vec, label = paste("LGLSXP FALSE n =", N))
  }
  
  
  for (N in seq(0, 73)) {
    # print(N)
    vec <- rep(NA, N)
    enc <- zap_write(vec)
    result <- zap_read(enc)
    expect_identical(result, vec, label = paste("LGLSXP NA n =", N))
  }
  
  
  
  
})
