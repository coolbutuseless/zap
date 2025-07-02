test_that("CLOSXP works", {
  result <- zap_write(mean) |> zap_read()
  expect_identical(result, mean)
})
