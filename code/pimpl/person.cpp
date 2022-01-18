#include "person.hpp"

#include <iostream>
#include <memory>
#include <string>

#include "basic_info.hpp"

namespace cpp_idioms {

struct Person::Impl {
  std::string name;
  std::string id;
  BasicInfo basic_info;
};

Person::Person() : pimpl_(std::make_unique<Impl>()) {}
Person::~Person() = default;
Person::Person(Person&& rhs) = default;
Person& Person::operator=(Person&& rhs) = default;

Person::Person(const Person& rhs)
    : pimpl_{std::make_unique<Impl>(*rhs.pimpl_)} {}
Person& Person::operator=(const Person& rhs) {
  *pimpl_ = *rhs.pimpl_;
  return *this;
};

void Person::Print() const {
  std::cout << "name: " << pimpl_->name << "; "
            << "id: " << pimpl_->id << "\n";

  std::cout << pimpl_->basic_info << "\n";
}

void Person::SetName(std::string name) { pimpl_->name = name; }
void Person::SetId(std::string id) { pimpl_->id = id; }

}