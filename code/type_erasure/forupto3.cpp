#include <functional>
#include <iostream>
#include <vector>

void ForUpTo(int n, std::function<void(int)> f) {
  for (int i = 0; i < n; ++i) {
    f(i);
  }
}

void PrintInt(int n) { std::cout << n << ' '; }
int main() {
  ForUpTo(5, PrintInt);

  std::vector<int> v;
  ForUpTo(5, [&v](int n) { v.push_back(n); });
  std::cout << '\n';
  return 0;
}