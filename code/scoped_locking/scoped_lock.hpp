#pragma once

#include <mutex>
#include <tuple>

namespace cpp_idioms {

template <int Index>
struct TryLockImpl {
  template <typename... Lock>
  static void DoTryLock(std::tuple<Lock&...>& locks, int& index) {
    index = Index;
    auto lock = std::unique_lock(std::get<Index>(locks), std::try_to_lock);
    if (lock.owns_lock()) {
      if constexpr (Index + 1 < sizeof...(Lock)) {
        TryLockImpl<Index + 1>::DoTryLock(locks, index);
      } else {
        index = -1;
      }
      if (index == -1) {
        lock.release();
      }
    }
  }
};

template <typename Lock1, typename Lock2, typename... Lock3>
void Lock(Lock1& l1, Lock2& l2, Lock3&... l3) {
  while (true) {
    std::unique_lock<Lock1> first(l1);
    int index = 0;
    if constexpr (sizeof...(Lock3)) {
      auto locks = std::tie(l2, l3...);
      TryLockImpl<0>::DoTryLock(locks, index);
    } else {
      auto lock = std::unique_lock(l2, std::try_to_lock);
      if (lock.owns_lock()) {
        index = -1;
        lock.release();
      }
    }

    if (index == -1) {
      first.release();
      return;
    }
  }
}

template <typename... MutexType>
class ScopedLock {
 public:
  explicit ScopedLock(MutexType&... ms) : mutexs_(std::tie(ms...)) {
    // std::lock(ms...);
    Lock(ms...);
  }

  // calling thread owns mutex
  explicit ScopedLock(std::adopt_lock_t, MutexType&... ms) noexcept
      : mutexs_(std::tie(ms...)) {}
  ~ScopedLock() {
    std::apply(
        [](MutexType&... ms) {
          {
            (ms.unlock(), ...);
            // [[maybe_unused]] char i[] = {(ms.unlock(), '0')...};
          }
        },
        mutexs_);
  }

  ScopedLock(const ScopedLock&) = delete;
  ScopedLock& operator=(const ScopedLock&) = delete;

 private:
  std::tuple<MutexType&...> mutexs_;
};

template <>
class ScopedLock<> {
 public:
  explicit ScopedLock() = default;
  explicit ScopedLock(std::adopt_lock_t) noexcept {}
  ~ScopedLock() = default;

  ScopedLock(const ScopedLock&) = delete;
  ScopedLock& operator=(const ScopedLock&) = delete;
};

template <typename MutexType>
class ScopedLock<MutexType> {
 public:
  explicit ScopedLock(MutexType& m) : mutex_(m) { mutex_.lock(); }
  // calling thread owns mutex
  explicit ScopedLock(std::adopt_lock_t, MutexType& m) noexcept : mutex_(m) {}
  ~ScopedLock() { mutex_.unlock(); }

  ScopedLock(const ScopedLock&) = delete;
  ScopedLock& operator=(const ScopedLock&) = delete;

 private:
  MutexType& mutex_;
};

}  // namespace cpp_idioms