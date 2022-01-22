#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

// prints the explanatory string of an exception. If the exception is nested,
// recurses to print the explanatory of the exception it holds
void print_exception(const std::exception& e, int level = 0) {
  std::cerr << std::string(level, ' ') << "exception: " << e.what() << '\n';
  try {
    std::rethrow_if_nested(e);
  } catch (const std::exception& nestedException) {
    print_exception(nestedException, level + 1);
  } catch (...) {
  }
}

// sample function that catches an exception and wraps it in a nested exception
void open_file(const std::string& s) {
  try {
    std::ifstream file(s);
    file.exceptions(std::ios_base::failbit);
  } catch (...) {
    std::throw_with_nested(std::runtime_error("Couldn't open " + s));
    // throw std::runtime_error("Couldn't open " + s);
  }
}

// sample function that catches an exception and wraps it in a nested exception
void run() {
  try {
    open_file("nonexistent.file");
  } catch (...) {
    std::throw_with_nested(std::runtime_error("run() failed"));
    // throw std::runtime_error("run() failed");
  }
}

// runs the sample function above and prints the caught exception
int main() {
  try {
    run();
  } catch (const std::exception& e) {
    print_exception(e);
  }
}