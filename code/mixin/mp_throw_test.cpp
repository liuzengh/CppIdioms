#include "mp_throw.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

void Foo() { MP_THROW(std::logic_error("oops")); }

int main() {
  try {
    Foo();
  } catch (const std::exception& e) {
    if (auto t = dynamic_cast<const cpp_idioms::Trace*>(&e)) {
      std::cout << "Exception-" << e.what()
                << " happend at: " << t->GetStackTrace() << "\n";
    }
  }
  return 0;
}
