cc_binary(
  name = "static_poly",
  srcs = ["static_poly.cpp"]
)

cc_library(
  name = "object_counter",
  hdrs = ["object_counter.hpp"],

)

cc_binary(
  name = "object_counter_test",
  srcs = ["object_counter_test.cpp"],
  deps = [":object_counter"]
)

cc_library (
  name = "linked_list",
  srcs = ["linked_list.hpp"]
)

cc_test(
  name = "linked_list_unittest",
  size = "small",
  srcs = ["linked_list_unittest.cpp"],
  deps = [":linked_list",
          "//utils:macros",
          "@com_google_googletest//:gtest_main"],
)