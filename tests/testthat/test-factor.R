
test_that("FACTORSXP works", {
  
  src <- c(letters, LETTERS)
  
  for (nlevels in seq_along(src)) {
    # cat("nlevels: ", nlevels, "nbits: ", ceiling(log2(nlevels)), "\n")
    for (N in c(1, 10, 100)) {
      vec <- as.factor(sample(src[seq(nlevels)], N, TRUE))
      enc <- zap_write(vec)
      res <- zap_read(enc)
      expect_identical(res, vec)
    }
  }
  
})


test_that("FACTORSXP with NAs work", {
  
  src <- c(letters, LETTERS)
  
  for (nlevels in seq_along(src)) {
    # cat("nlevels: ", nlevels, "nbits: ", ceiling(log2(nlevels)), "\n")
    for (N in c(1, 10, 100)) {
      vec <- as.factor(sample(src[seq(nlevels)], N, TRUE))
      vec[1] <- NA
      enc <- zap_write(vec)
      res <- zap_read(enc)
      expect_identical(res, vec)
    }
  }
  
  
  
})


test_that("factor with NAs - penguins", {
  vec <- penguins$sex[1:4]
  enc <- zap_write(vec)
  dec <- zap_read(enc)
  dec
  expect_identical(dec, vec)
})



test_that("Large factors work as INTSXP", {
  
  src <- c(
    letters, 
    LETTERS,
    paste0(letters, 0),
    paste0(letters, 1),
    paste0(letters, 2),
    paste0(letters, 3),
    paste0(letters, 4),
    paste0(letters, 5),
    paste0(letters, 6),
    paste0(letters, 7),
    paste0(letters, 8),
    paste0(letters, 9)
  )
  
  vec <- as.factor(sample(src, 10000, T))
  stopifnot(nlevels(vec) > 256)
  
  enc <- zap_write(vec)
  res <- zap_read(enc)
  expect_identical(res, vec)
  
})
