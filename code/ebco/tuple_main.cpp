#include <iostream>

#include "tuple.hpp"

class A {
 public:
  A() {
    std::cout << "A()"
              << "\n";
  }
};

struct B {
  B() {
    std::cout << "B()"
              << "\n";
  }
};

int main() {
  cpp_idioms::Tuple<A, char, A, char, B> t;
  std::cout << "sizeof(t1): " << sizeof(t) << "\n";
  return 0;
}