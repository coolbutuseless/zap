
# all SEXP types: zero-length, NA, empty, Inf, NaN

test_that("all specials work", {
  
  N <- 1000
  
  # lgl
  set.seed(1)
  for (sp in list(NA, logical(0), sample(c(T, F), N, TRUE), rep(NA, N))) {
    for (threshold in c(0, N + 1)) {
        for (lgl in c('raw', 'packed')) {
          expect_identical(
            zap_write(sp, lgl = lgl, lgl_threshold = threshold) |> zap_read(),
            sp
          )
        }
    }
  }
  
  # int
  set.seed(1)
  for (sp in list(NA_integer_, integer(0), sample(N), rep(NA_integer_, N))) {
    for (threshold in c(0, N + 1)) {
        for (int in c('raw', 'zzshuf', 'deltaframe')) {
          expect_identical(
            zap_write(sp, int = int, int_threshold = threshold) |> zap_read(),
            sp
          )
      }
    }
  }
  
  # fct
  set.seed(1)
  for (sp in list(as.factor(sample(letters[1:10], N, TRUE)))) {
    for (threshold in c(0, N + 1)) {
        for (fct in c('raw', 'packed')) {
          expect_identical(
            zap_write(sp, fct = fct, fct_threshold = threshold) |> zap_read(),
            sp
          )
        }
    }
  }
  
  # real
  set.seed(1)
  for (sp in list(NA_real_, double(0), NA_complex_, complex(0), NaN, Inf, -Inf,
                  rnorm(N), rep(NA_real_, N), rep(NA_complex_, N))) {
    for (threshold in c(0, N + 1)) {
        for (dbl in c('raw', 'shuffle', 'delta_shuffle', 'alp')) {
          expect_identical(
            zap_write(sp, dbl = dbl, dbl_threshold = threshold) |> zap_read(),
            sp
          )
        }
    }
  }
  
  # character
  set.seed(1)
  for (sp in list(NA_character_, "", character(0), sample(letters, N, TRUE),
                  rep(NA_character_, N), rep("", N))) {
    for (threshold in c(0, N + 1)) {
        for (str in c('raw', 'mega')) {
          expect_identical(
            zap_write(sp, str = str, str_threshold = threshold) |> zap_read(),
            sp
          )
        }
    }
  }
  
  
})
