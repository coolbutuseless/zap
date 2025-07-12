
test_that("objdf debugging works", {
  
  res <- zap_write(mtcars, verbosity = 64)
  expect_true(is.data.frame(res))
  
  
})
