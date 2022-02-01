#include <any>
#include <iostream>

struct Pod {
  int i{0};
  float j{0.0};
  double k{0.1};
  Pod() = default;
};
int main() {
  std::any a;
  std::any b = 4.3;
  if (b.type() == typeid(double)) {
    auto d = (std::any_cast<double>(b));
  };

  a = 42;
  a = Pod{};
  std::cout << "a.Type():" << a.type().name() << std::endl;
  return 0;
}
