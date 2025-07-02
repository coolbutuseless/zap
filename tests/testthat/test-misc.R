
test_that("opts and version work", {
  
  expect_true(is.integer(zap_version()))
  
  expect_true(is.list(zap_opts(zstd_level = 3)))
  
})
