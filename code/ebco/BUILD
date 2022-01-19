cc_binary(
  name = "empty_class",
  srcs = ["empty_class.cpp"]
)

cc_binary(
  name = "ebco_success",
  srcs = ["ebco_success.cpp"]
)

cc_binary(
  name = "ebco_fail",
  srcs = ["ebco_fail.cpp"]
)

cc_library(
  name = "tuple_element1",
  hdrs = ["tuple_element1.hpp"]
)

cc_library(
  name = "tuple_storage1",
  hdrs = ["tuple_storage1.hpp"],
  deps = [":tuple_element1"]
)

cc_binary(
  name = "compressed_tuple1",
  srcs = ["compressed_tuple1.cpp"],
  deps = [":tuple_storage1"]
)


cc_library(
  name = "tuple_element2",
  hdrs = ["tuple_element2.hpp"]
)

cc_library(
  name = "tuple_storage2",
  hdrs = ["tuple_storage2.hpp"],
  deps = [":tuple_element2"]
)

cc_binary(
  name = "compressed_tuple2",
  srcs = ["compressed_tuple2.cpp"],
  deps = [":tuple_storage2"]
)

cc_library (
  name = "tuple",
  hdrs = ["tuple.hpp"]
)

cc_binary (
  name = "tuple_main",
  srcs = ["tuple_main.cpp"],
  deps = ["tuple"]
)