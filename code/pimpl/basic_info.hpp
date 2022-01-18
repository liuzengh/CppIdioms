#pragma once

#include <iostream>

namespace cpp_idioms {

class BasicInfo {
 public:
  bool GetGender() const { return gender_; }
  void SetGender(bool gender) { gender_ = gender; }
  int GetAge() const { return age_; };
  [[nodiscard]] bool SetAge(int age);
  float GetHeight() const { return height_; }
  [[nodiscard]] bool SetHeight(float height);
  float GetWeight() const { return weight_; }
  [[nodiscard]] bool SetWeight(float weight);

 private:
  bool gender_{true};
  int age_{18};
  float height_{175};
  float weight_{65};
};

std::ostream& operator<<(std::ostream& out, const BasicInfo& basic_info) {
  out << "gender: " << (basic_info.GetGender() ? "male" : "female") << "\n";
  out << "age: " << basic_info.GetAge() << "\n";
  out << "weigt: " << basic_info.GetWeight() << "kg"
      << "\n";
  out << "height: " << basic_info.GetHeight() << "cm";
  return out;
}

}  // namespace cpp_idioms