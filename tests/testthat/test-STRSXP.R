

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



if (FALSE) {
  
  nms <- rownames(mtcars)
  
  for (i in c(seq_len(32), 64, 128, 256, 512, 1024)) {
    set.seed(1)
    ref <- sample(nms, i, TRUE)
    l1 <- zap_write(ref, str = 'mega', compress = 'none') |> length()
    l2 <- zap_write(ref, str = 'dict', compress = 'none') |> length()
    cat(sprintf("% 5i % 7i % 7i\n", i, l1, l2))
  }  
  
  for (i in c(seq_len(32), 64, 128, 256, 512, 1024)) {
    set.seed(1)
    ref <- sample(nms, i, TRUE)
    l1 <- zap_write(ref, str = 'mega', compress = 'zstd') |> length()
    l2 <- zap_write(ref, str = 'dict', compress = 'zstd') |> length()
    cat(sprintf("% 5i % 7i % 7i\n", i, l1, l2))
  }  
  
  
  
}








