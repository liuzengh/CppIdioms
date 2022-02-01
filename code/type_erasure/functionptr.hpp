#pragma once

// primary template
template <typename Signature>
class FunctionPtr;

// partial specialization
template <typename R, typename... Args>
class FunctionPtr<R(Args...)> {
 private:
  FunctorBridge<R, Args...>* bridge_;
};
