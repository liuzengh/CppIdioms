#include <iostream>

class EmptyOne {};
class EmptyTwo : public EmptyOne {};
class EmptyThree : public EmptyTwo {};

int main() {
  std::cout << "sizeof(Empty): " << sizeof(EmptyOne) << '\n';
  std::cout << "sizeof(EmptyTwo): " << sizeof(EmptyTwo) << '\n';
  std::cout << "sizeof(EmptyThree): " << sizeof(EmptyThree) << '\n';
  return 0;
}