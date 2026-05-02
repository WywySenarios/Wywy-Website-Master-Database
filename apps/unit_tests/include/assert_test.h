#define assert_true(value, msg)                                                \
  ({                                                                           \
    if (value != 1) {                                                          \
      puts(msg);                                                               \
    }                                                                          \
  })

#define assert_false(value, msg)                                               \
  ({                                                                           \
    if (value != 0) {                                                          \
      puts(msg);                                                               \
    }                                                                          \
  })