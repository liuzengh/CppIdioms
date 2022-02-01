
#include "any.hpp"

#include <iostream>
#include <vector>

struct Pod {
  int i{0};
  float j{0.0};
  double k{0.1};
  Pod() = default;
};

int main() {
  Any a;
  Any b = 4.3;
  std::cout << "b.Type():" << b.Type().name() << std::endl;
  if (b.Type() == typeid(double)) {
    auto d = (AnyCast<double>(b));
    std::cout << "d:" << d << std::endl;
  };

  b = 42;
  std::cout << "b.Type():" << b.Type().name() << std::endl;

  b = Pod{};
  std::cout << "b.Type():" << b.Type().name() << std::endl;

  return 0;
}