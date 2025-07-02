

test_that("ENVSXP works", {

  set.seed(1)
  e <- new.env(parent = .GlobalEnv)
  e$a <- TRUE
  e$b <- FALSE
  enc <- zap_write(e);
  result <- zap_read(enc)
  expect_identical(result, e)
  
})





test_that("Nested ENVSXP works", {
  
  set.seed(1)
  e1 <- new.env(parent = .GlobalEnv)
  e2 <- new.env(parent = e1)
  e2$a <- TRUE
  e2$b <- FALSE
  enc <- zap_write(e2);
  result <- zap_read(enc)
  expect_equal(result, e2)
  
})




#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# References to the same environment in the source, must have
# equivalent matching environments in the final result
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("Environment references work", {

  set.seed(1)
  e <- new.env(parent = .GlobalEnv)
  f <- new.env(parent = .GlobalEnv)
  e$a <- TRUE
  e$b <- FALSE
  
  
  # Single environment works
  enc <- zap_write(e);
  result <- zap_read(enc)
  expect_identical(result, e)
  
  # list combo of environments work
  ef <- list(e, f)
  enc <- zap_write(ef);
  result <- zap_read(enc)
  expect_identical(result, ef)
  
  
  
  # list combination of environemnts works
  ll <- list(e, e)

  enc <- zap_write(ll);
  result <- zap_read(enc)
  expect_identical(result, ll)

  # Now ensure that the two environment points are the same at the start
  expect_true(identical(ll[[1]], ll[[2]]))


  # And the same environments in the final result
  expect_true(identical(result[[1]], result[[2]]))
})




#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# References to the same environment in the source, must have
# equivalent matching environments in the final result
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
test_that("More environment references work", {

  set.seed(1)
  e <- new.env(parent = .GlobalEnv)
  e$a <- TRUE
  e$b <- FALSE

  f <- new.env(parent = e)
  f$a <- 1
  f$b <- 2

  ll <- list(e, e, f, f, e)

  enc <- zap_write(ll);
  result <- zap_read(enc)
  expect_identical(result, ll)

  # Now ensure that the two environment points are the same at the start
  expect_true(identical(ll[[1]], ll[[2]]))


  # And the same environments in the final result
  expect_true(identical(result[[1]], result[[2]]))
  expect_true(identical(result[[1]], result[[5]]))
  expect_true(identical(result[[3]], result[[4]]))

  expect_false(identical(result[[2]], result[[3]]))
})



test_that("Special environments work", {
  
  x <- list(.GlobalEnv, baseenv(), emptyenv(), findPackageEnv('package:stats'), mean)
  
  enc <- zap_write(x)
  result <- zap_read(enc)
  expect_identical(result, x)
  
  
})







