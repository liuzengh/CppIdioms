#pragma once

#include <memory>
#include <string>

namespace cpp_idioms {

class Person {
 public:
  Person();
  ~Person();
  Person(Person&& rhs);
  Person& operator=(Person&& rhs);
  Person(const Person& rhs);
  Person& operator=(const Person& rhs);

  void Print() const;
  void SetName(std::string name);
  void SetId(std::string id);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace cpp_idioms
