

test_that("file io works", {
  
  tmp <- tempfile()
  zap_write(mtcars, tmp)
  res <- zap_read(tmp)
  expect_identical(res, mtcars)
  
})

test_that("large file io works", {
  
  set.seed(1)
  df <- mtcars[sample(nrow(mtcars), 100000, T), ]

  tmp <- tempfile()
  zap_write(df, tmp)
  res <- zap_read(tmp)
  expect_identical(res, df)
  
})
