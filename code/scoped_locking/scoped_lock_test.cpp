#include "scoped_lock.hpp"

#include <algorithm>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace {
constexpr int kTimes = 100000;
}

void InitVector(std::vector<int>& v1, std::vector<int>& v2) {
  for (int i = 0; i < kTimes; ++i) {
    v1.emplace_back(i);
    v2.emplace_back(v1[i]);
  }
}

int main() {
  std::vector<int> v1;
  std::mutex mutex_v1;
  std::vector<int> v2;
  std::timed_mutex mutex_v2;

  {
    cpp_idioms::ScopedLock<std::mutex, std::timed_mutex> lg(mutex_v1, mutex_v2);
    InitVector(v1, v2);
  }

  auto fut1 = std::async(std::launch::async, [&v1, &v2, &mutex_v1, &mutex_v2] {
    cpp_idioms::ScopedLock lg(mutex_v1, mutex_v2);
    InitVector(v1, v2);
  });
  fut1.get();

  std::cout << std::thread::hardware_concurrency() << std::endl;

  std::cout << (std::is_sorted(v1.begin(), v1.begin() + v1.size() / 2) ? "good"
                                                                       : "bad")
            << "\n";

  // {
  //   std::scoped_lock lg(allIssuesMx, openIssuesMx);
  //   // manipulate both allIssues and openIssues
  // }

  // // lock both issue lists:
  // {
  //   // lock with deadlock avoidance
  //   std::lock(allIssuesMx, openIssuesMx);
  //   std::lock_guard<std::mutex> lg1(allIssuesMx, std::adopt_lock);
  //   std::lock_guard<std::timed_mutex> lg2(openIssuesMx, std::adopt_lock);
  //   // manipulate both allIssues and openIssues
  // }

  // // lock both issue lists:
  // {
  //   // Note: deadlock avoidance algorithm used
  //   std::lock(allIssuesMx, openIssuesMx);
  //   std::scoped_lock lg(std::adopt_lock, allIssuesMx, openIssuesMx);
  //   // manipulate both allIssues and openIssues
  // }

  return 0;
}
