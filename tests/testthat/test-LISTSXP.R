



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





test_that("data.frame works", {
  
  df <- head(mtcars, 3)
  
  enc <- zap_write(df)
  result <- zap_read(enc)
  attributes(result)
  expect_identical(result, df)
})






test_that("matrix works", {
  
  mat <- matrix(1:10, 2, 5)
  
  enc <- zap_write(mat)
  result <- zap_read(enc)
  attributes(result)
  expect_identical(result, mat)
})


