#pragma once

#include <memory>
#include <type_traits>

#include "functor_bridge.hpp"
#include "specific_functor_bridge.hpp"

// primary template
template <typename Signature>
class FunctionPtr;

// partial specialization
template <typename R, typename... Args>
class FunctionPtr<R(Args...)> {
 public:
  FunctionPtr() : bridge_{nullptr} {}

  FunctionPtr(const FunctionPtr&);

  FunctionPtr(FunctionPtr& other)
      : FunctionPtr(static_cast<FunctionPtr const&>(other)) {}
  FunctionPtr(FunctionPtr&& other) : bridge_{other.bridge_} {
    other.bridge_ = nullptr;
  }

  template <typename F>
  FunctionPtr(F&& f);

  // assignment operators:
  FunctionPtr& operator=(const FunctionPtr& other) {
    FunctionPtr temp(other);
    swap(*this, temp);
    return *this;
  }

  FunctionPtr& operator=(FunctionPtr&& other) {
    delete bridge_;
    bridge_ = other.bridge_;
    other.bridge_ = nullptr;
    return *this;
  }

  // construction and assignment from arbitrary function objects:
  template <typename F>
  FunctionPtr& operator=(F&& f) {
    FunctionPtr temp(std::forward<F>(f));
    swap(*this, temp);
    return *this;
  }

  // destructor:
  ~FunctionPtr() { delete bridge_; }

  friend bool operator==(const FunctionPtr& f1, const FunctionPtr& f2) {
    if (!f1 || !f2) {
      return !f1 && !f2;
    }
    return f1.bridge_->Equals(f2.bridge_);
  }
  friend bool operator!=(const FunctionPtr& f1, const FunctionPtr& f2) {
    return !(f1 == f2);
  }

  friend void swap(FunctionPtr& fp1, FunctionPtr& fp2) {
    std::swap(fp1.bridge_, fp2.bridge_);
  }

  explicit operator bool() const { return bridge_ == nullptr; }

  // invocation
  R operator()(Args... args) const;

 private:
  FunctorBridge<R, Args...>* bridge_;
};

template <typename R, typename... Args>
FunctionPtr<R(Args...)>::FunctionPtr(const FunctionPtr& other)
    : bridge_{nullptr} {
  if (other.bridge_) {
    bridge_ = other.bridge_->Clone();
  }
}

template <typename R, typename... Args>
R FunctionPtr<R(Args...)>::operator()(Args... args) const {
  return bridge_->Invoke(std::forward<Args>(args)...);
}

template <typename R, typename... Args>
template <typename F>
FunctionPtr<R(Args...)>::FunctionPtr(F&& f) : bridge_{nullptr} {
  using Functor = std::decay_t<F>;
  using Bridge = SpecificFunctorBridge<Functor, R, Args...>;
  bridge_ = new Bridge(std::forward<F>(f));
}