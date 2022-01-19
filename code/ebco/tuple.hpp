#pragma once

#include <type_traits>
#include <utility>

namespace cpp_idioms {

template <typename... Types>
class Tuple;

// basic case:
template <>
class Tuple<> {
  // no storage required
};

// recursive case:
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
 public:
  Tuple() = default;
  Tuple(const Head& head, const Tuple<Tail...>& tail)
      : head_(head), tail_(tail) {}

  template <typename VHead, typename... VTail>
  Tuple(VHead&& vhead, VTail&&... vtail)
      : head_(std::forward<VHead>(vhead)),
        tail_(std::forward<VTail>(vtail)...) {}

  template <typename VHead, typename... VTail,
            typename = std::enable_if_t<sizeof...(VTail) == sizeof...(Tail)>>
  Tuple(const Tuple<VHead, VTail...>& other)
      : head_(other.GetHead()), tail_(other.GetTail()) {}

  Head& GetHead() { return head_; }
  const Head& GetHead() const { return head_; }

  Tuple<Tail...>& GetTail() { return tail_; }
  const Tuple<Tail...>& GetTail() const { return tail_; }

 private:
  Head head_;
  Tuple<Tail...> tail_;
};

}  // namespace cpp_idioms