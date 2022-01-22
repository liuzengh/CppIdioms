#pragma once
#include <string>

namespace cpp_idioms {

class Trace {
 public:
  Trace(std::string file, int line, std::string func)
      : file_(file), line_(line), func_(func){};
  std::string GetStackTrace() const {
    return file_ + ":" + std::to_string(line_) + ":" + func_;
  };

 private:
  std::string file_;
  int line_;
  std::string func_;
};

template <class Exception>
struct Traceable : public Exception, public Trace {
  Traceable(Exception e, std::string file, int line, std::string func)
      : Exception(std::move(e)), Trace(file, line, func) {}
};

template <class Exception>
auto MakeTraceable(Exception e, std::string file, int line, std::string func) {
  return Traceable<Exception>(std::move(e), file, line, func);
}

}  // namespace cpp_idioms

#define MP_THROW(e) \
  throw cpp_idioms::MakeTraceable((e), __FILE__, __LINE__, __FUNCTION__)
