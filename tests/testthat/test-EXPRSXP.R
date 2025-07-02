

test_that("EXPRSXP works", {
  
  
  e <- as.expression(c('a'))
  enc <- zap_write(e)  
  result <- zap_read(enc)
  expect_identical(result, e)
  
  e <- as.expression(c('aa', 'bb', "cc"))
  enc <- zap_write(e)
  result <- zap_read(enc)
  expect_identical(result, e)
  
  
  e <- expression(1 + 1)
  enc <- zap_write(e)
  result <- zap_read(enc)
  expect_identical(result, e)
  
  
  e <- expression(2 * 3 + a)
  enc <- zap_write(e)
  result <- zap_read(enc)
  expect_identical(result, e)
  
  
  e <- quote(a + 3 * mean(x))
  enc <- zap_write(e)
  result <- zap_read(enc)
  expect_identical(result, e)
  
  
  # zap_write(expression(1 + 1), tmp)  
})
