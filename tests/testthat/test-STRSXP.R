

test_that("special strings work", {
  
  refs <- list(
    NA_character_,
    "",
    character(0)
  )
  
  for (ref in refs) {
    enc <- zap_write(ref)
    result <- zap_read(enc)
    expect_identical(result, ref)
  }  
})

test_that("strings works", {
  ref <- c("", "a", "ab", "abc")
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
})


test_that("multiple empty works", {
  ref <- c("", "", "", "")
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
})


test_that("multiple NA works", {
  ref <- c(NA_character_, NA_character_, NA_character_)
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
})


test_that("corner caseworks", {
  ref <- c("", "", "", "", NA_character_, "", "", "", "")
  enc <- zap_write(ref)
  result <- zap_read(enc)
  expect_identical(result, ref)
})




test_that("results identical to input - raw", {
  ref <- c("", "a", "ab", "abc")
  
  for (method in c('raw', 'mega', 'dict')) {
    enc <- zap_write(ref, str = method)
    result <- zap_read(enc)
    expect_identical(result, ref)
  }
})
