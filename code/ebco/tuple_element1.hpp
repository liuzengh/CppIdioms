#pragma once

namespace cpp_idioms {

template <unsigned int Height, typename T>
class TupleElement {
 private:
  T value_;

 public:
  TupleElement() = default;
  template <typename U>
  TupleElement(U&& other) : value_(std::forward<U>(other)){};
  T& Get() { return value_; }
  const T& Get() const { return value_; }
};

}  // namespace cpp_idioms