#pragma once

template <typename R, typename... Args>
class FunctorBridge {
 public:
  virtual ~FunctorBridge() {}
  virtual FunctorBridge* Clone() const = 0;
  virtual R Invoke(Args... arg) const = 0;
  virtual bool Equals(const FunctorBridge* fb) const = 0;
};