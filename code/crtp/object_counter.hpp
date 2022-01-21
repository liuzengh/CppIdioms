#pragma once

#include <cstddef>

namespace cpp_idioms {

template <class Derived>
class ObjectCounter {
 private:
  inline static std::size_t count{0};

 protected:
  ObjectCounter() { ++count; }
  ObjectCounter(const ObjectCounter&) { ++count; }
  ObjectCounter(ObjectCounter&&) { ++count; }
  ~ObjectCounter() { --count; }

 public:
  static std::size_t CountLive() { return count; }
};

}  // namespace cpp_idioms
