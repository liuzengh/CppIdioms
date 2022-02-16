cc_library(
  name = "any",
  hdrs = ["any.hpp"]
)

cc_binary(
  name = "forupto1",
  srcs = ["forupto1.cpp"]
)

cc_binary(
  name = "forupto3",
  srcs = ["forupto3.cpp"]
)

cc_binary(
  name = "forupto4",
  srcs = ["forupto4.cpp"],
  deps = [":function_ptr"]
)

cc_binary(
  name = "any_test",
  srcs = ["any_test.cpp"],
  deps = [":any"]
)

cc_binary(
  name = "any_example",
  srcs = ["any_example.cpp"],
)


cc_library(
  name = "function_ptr",
  hdrs = ["function_ptr.hpp"],
  deps = [":functor_bridge", "specific_functor_bridge"]
)


cc_library(
  name = "functor_bridge",
  hdrs = ["functor_bridge.hpp"]
)


cc_library(
  name = "specific_functor_bridge",
  hdrs = ["specific_functor_bridge.hpp"],
  deps = [":try_equals"]
)


cc_library(
  name = "try_equals",
  hdrs = ["try_equals.hpp"],
  deps = [":is_equality_comparable"]
)


cc_library(
  name = "is_equality_comparable",
  hdrs = ["is_equality_comparable.hpp"]
)
