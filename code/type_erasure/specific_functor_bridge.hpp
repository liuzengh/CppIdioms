#pragma once

#include <memory>

#include "try_equals.hpp"

template <typename Functor, typename R, typename... Args>
class SpecificFunctorBridge : public FunctorBridge<R, Args...> {
 public:
  template <typename FunctorFwd>
  SpecificFunctorBridge(FunctorFwd&& functor)
      : functor_(std::forward<FunctorFwd>(functor)) {}

  virtual SpecificFunctorBridge* Clone() const override {
    return new SpecificFunctorBridge(functor_);
  }

  virtual R Invoke(Args... args) const override {
    return functor_(std::forward<Args>(args)...);
  }

  virtual bool Equals(const FunctorBridge<R, Args...>* fb) const override {
    if (auto spec_fb = dynamic_cast<const SpecificFunctorBridge*>(fb)) {
      return TryEquals<Functor>::Equals(functor_, spec_fb->functor_);
    }
    // functors with different types are never equal:
    return false;
  }

 private:
  Functor functor_;
};