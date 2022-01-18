#include "basic_info.hpp"

namespace cpp_idioms {

bool BasicInfo::SetAge(int age) {
  if (age < 0) return false;
  age_ = age;
  return true;
}

bool BasicInfo::SetHeight(float height) {
  if (height < 0) return false;
  height_ = height;
  return true;
}

bool BasicInfo::SetWeight(float weight) {
  if (weight < 0) return false;
  weight_ = weight;
  return true;
}

}  // namespace cpp_idioms
