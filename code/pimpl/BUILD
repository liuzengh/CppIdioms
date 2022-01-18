cc_library(
  name = "basic_info",
  hdrs = ["basic_info.hpp"],
  srcs = ["basic_info.cpp"]
)

cc_library(
  name = "person",
  hdrs = ["person.hpp"],
  srcs = ["person.cpp"],
  deps = [":basic_info"]
)

cc_binary(
  name = "main",
  srcs = ["main.cpp"],
  deps = [":person"]
)