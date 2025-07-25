


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


test_that("list = 'reference' option works", {
  
  
  binary_tree <- function(depth = 0) {
    if(depth == 0) {
      list()
    } else { 
      a <- binary_tree(depth - 1); 
      list(a, a) 
    }
  }
  
  b <- binary_tree(1)
  expect_equal(
    address(b[[1]]), 
    address(b[[2]])
  )
  
  v1 <- zap_write(b) |> zap_read()
  expect_false(
    address(v1[[1]]) ==  address(v1[[2]])
  )
  
  
  v2 <- zap_write(b, list = 'reference') |> zap_read()
  expect_true(
    address(v2[[1]]) ==  address(v2[[2]])
  )
  
})