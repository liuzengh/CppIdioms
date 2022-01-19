#include <iostream>

#include "tuple_storage2.hpp"

struct A {
  A() { std::cout << "A()" << '\n'; }
};

struct B {
  B() { std::cout << "B()" << '\n'; }
};

int main() {
  cpp_idioms::Tuple<A, char, A, char, B> t1;
  std::cout << sizeof(t1) << " bytes" << '\n';
  return 0;
}