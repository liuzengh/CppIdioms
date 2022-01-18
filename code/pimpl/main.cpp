#include "person.hpp"

int main() {
  cpp_idioms::Person person;
  person.SetName("liuzengh");
  person.SetId("007");
  person.Print();
  return 0;
}