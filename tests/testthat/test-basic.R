

test_that("atomic i/o works", {
  
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
  
  for (ref in refs) {
    enc <- zap_write(ref)
    res <- zap_read(enc)
    expect_identical(res, ref)
  }
  
  
  enc <- zap_write(refs)
  res <- zap_read(enc)
  expect_identical(res, refs)
  
  
})



test_that("empty list works", {
  ref <- list()
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
})





test_that("strings works", {
  ref <- c("a", "ab", "abc")
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
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





test_that("data.frame works", {
  
  df <- head(mtcars, 3)
  
  enc <- zap_write(df)
  result <- zap_read(enc)
  attributes(result)
  expect_identical(result, df)
})



test_that("data.frame works from file - compressed", {
  
  df <- head(mtcars, 3)
  tmp <- tempfile()
  zap_write(df, tmp)
  result <- zap_read(tmp)
  attributes(result)
  expect_identical(result, df)
})


test_that("data.frame works from file - uncompressed", {
  
  df <- head(mtcars, 3)
  tmp <- tempfile()
  zap_write(df, tmp)
  result <- zap_read(tmp)
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




test_that("uncompressed works", {
  
  
  enc <- zap_write(mtcars, compress = 'none')
  expect_true(length(enc) > 3000) # usually 3500
  result <- zap_read(enc)
  expect_identical(result, mtcars)
})



