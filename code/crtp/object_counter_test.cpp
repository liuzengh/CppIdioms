#include "object_counter.hpp"

#include <iostream>

template <typename T>
class MyVector : public cpp_idioms::ObjectCounter<MyVector<T>> {};

class MyCharString : public cpp_idioms::ObjectCounter<MyCharString> {};

int main() {
  MyVector<int> v1, v2;
  MyCharString s1;

  std::cout << "number of MyVector<int>: " << MyVector<int>::CountLive()
            << "\n";

  std::cout << "number of MyCharString: " << MyCharString::CountLive() << "\n";
  return 0;
}