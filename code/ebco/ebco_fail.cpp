#include <iostream>

class EmptyOne {};
class EmptyTwo : public EmptyOne {};
class NonEmpty : public EmptyOne, public EmptyTwo {};

int main() {
  std::cout << "sizeof(Empty): " << sizeof(EmptyOne) << '\n';
  std::cout << "sizeof(EmptyTwo): " << sizeof(EmptyTwo) << '\n';
  std::cout << "sizeof(EmptyThree): " << sizeof(NonEmpty) << '\n';
  return 0;
}