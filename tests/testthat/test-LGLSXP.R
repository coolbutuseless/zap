


test_that("logical bit packing works", {
  
  set.seed(1)
  
  
  for (N in seq(0, 71)) {
    # print(N)
    vec <- sample(c(TRUE, FALSE), N, replace = TRUE)
    enc <- zap_write(vec)
    result <- zap_read(enc)
    expect_identical(result, vec)
  }
})
