#pragma once

#include "tuple_element2.hpp"

namespace cpp_idioms {

template <typename... Types>
class Tuple;

// basis case:
template <>
class Tuple<> {
  // no storage required
};

// recursive case:
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> : private TupleElement<sizeof...(Tail), Head>,
                             private Tuple<Tail...> {
  using HeadElt = TupleElement<sizeof...(Tail), Head>;

 public:
  Head& GetHead() { return static_cast<HeadElt*>(this)->Get(); }
  Head const& GetHead() const {
    return static_cast<HeadElt const*>(this)->Get();
  }
  Tuple<Tail...>& GetTail() { return *this; }
  Tuple<Tail...> const& GetTail() const { return *this; }
};

}  // namespace cpp_idioms