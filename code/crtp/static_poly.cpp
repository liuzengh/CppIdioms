#include <iostream>

template <class Derived>
class Y {
 public:
  void Name() { (static_cast<Derived*>(this))->Name(); }
};

class X1 : public Y<X1> {
 public:
  void Name() {
    std::cout << "X1"
              << "\n";
  }
};

class X2 : public Y<X2> {
 public:
  void Name() {
    std::cout << "X2"
              << "\n";
  }
};

int main() {
  Y<X1> base1;
  Y<X2> base2;
  base1.Name();
  base2.Name();

  X1 x1;
  X2 x2;
  x1.Name();
  x2.Name();
  return 0;
}