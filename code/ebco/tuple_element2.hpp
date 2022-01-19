#pragma once

#include <type_traits>

namespace cpp_idioms {

template <unsigned Height, typename T,
          bool = std::is_class_v<T> && !std::is_final_v<T>>
class TupleElement;

template <unsigned Height, typename T>
class TupleElement<Height, T, false> {
  T value;

 public:
  TupleElement() = default;
  template <typename U>
  TupleElement(U&& other) : value(std::forward<U>(other)) {}
  T& get() { return value; }
  T const& get() const { return value; }
};

template <unsigned Height, typename T>
class TupleElement<Height, T, true> : private T {
 public:
  TupleElement() = default;
  template <typename U>
  TupleElement(U&& other) : T(std::forward<U>(other)) {}
  T& get() { return *this; }
  T const& get() const { return *this; }
};

}  // namespace cpp_idioms