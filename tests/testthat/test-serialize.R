
test_that("using R serialization works", {
  
  # the 'mean' function includes bytecode which is currently handled
  # only by R's serialization
  
  result <- zap_write(mean) |> zap_read()
  expect_identical(result, mean)
  
  
})
