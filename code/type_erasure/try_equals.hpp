
#pragma once
#include <exception>

#include "is_equality_comparable.hpp"

template <typename T, bool EqComparable = IsEqualityComparable<T>::value>
struct TryEquals {
  static bool Equals(T const& x1, T const& x2) { return x1 == x2; }
};

class NotEqualityComparable : public std::exception {};

template <typename T>
struct TryEquals<T, false> {
  static bool Equals(const T& x1, const T& x2) {
    throw NotEqualityComparable();
  }
};