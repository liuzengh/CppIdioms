#include <iostream>
#include <vector>

template <typename F>
void ForUpTo(int n, F f) {
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