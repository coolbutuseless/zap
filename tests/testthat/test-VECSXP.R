


test_that("empty list works", {
  ref <- list()
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
})



test_that("nested lists works", {
  ref <- list(a = list(1, 2, 3), b = list('a', 'b', 'c'), c = 3)
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
})




test_that("data.frame works", {
  
  df <- head(mtcars, 3)
  
  enc <- zap_write(df)
  result <- zap_read(enc)
  attributes(result)
  expect_identical(result, df)
})

