## C++编程技巧：Scoped Locking

Scoped Locking 是将RAII手法应用于locking的并发编程技巧。其具体做法就是在构造时获得锁，在析构时释放锁，目前Scoped Locking技巧在C++中有以下4种实现：

- std::lock_guard (c++11): 单个std::mutex(或std::shared_mutex) 
- std::unique_lock (c++11): 单个std::mutex(或std::shared_mutex), 用法比std::lock_guard更灵活
- std::shared_lock (c++14): 单个std::shared_mutex
- std::scoped_lock (c++17): 多个std::mutex(或std::shared_mutex)

对单个mutex上锁很简单，但是如何对多个mutex上锁呢？ c++17提供了std::scoped_lock可以对多个不同类型的mutex进行Scoped Locking。这里想来分析下std::scoped_lock的实现。
 
### ABBA死锁与std::scoped_lock的预防死锁策略

对多个mutex上锁可能会出现死锁，最常见的例子是ABBA死锁。假设存在两个线程（线程1和线程2）和两个锁（锁A和锁B），这两个线程的执行情况如下：

| 线程1         | 线程2        |
| -----------   | -----------  |
| 获得锁A       | 获得锁B      |
| 尝试获得锁B   | 尝试获得锁A   |
| 等待获得锁B   | 等待获得锁A   |
| ......        | ......       |

线程1在等待线程2持有的锁B，线程2在等待线程1持有的锁A，两个线程都不会释放其获得的锁; 因此，两个锁都不可用，将陷入死锁。

解决死锁最终都要打破资源依赖的循环，一般来说有两种思路：

1. 预防死锁：预防就是想办法不让线程进入死锁
2. 检测死锁和从死锁中恢复：如果预防死锁的代价比较高，而死锁出现的几率比较小，不如就先让其自由发展，在这个过程中提供一个检测手段来检查是否已经出现死锁，当检查出死锁之后就就想办法破坏存在死锁的必要条件，让线程从死锁中恢复过来。

感觉跟医病差不多......, “死锁”这种病还病得不浅呐。阅读std::scoped_lock源码可以知道，其采用了和std::lock相同的死锁预防算法来对多个mutex上锁。

std::scoped_lock的预防死锁策略很简单，假设要对n个mutex（mutex1, mutex2, ..., mutexn）上锁，那么每次只尝试对一个mutex上锁，只要上锁失败就立即释放获得的所有锁（方便让其他线程获得锁），然后重新开始上锁，处于一个循环当中，直到对n个mutex都上锁成功。这种策略是基本上是有效的，虽然有极小的概率出现“活锁”，例如上面的ABBA死锁中，线程1释放锁A的同一时刻时线程2又释放了锁B，然后这两个线程又同时分别获得了锁A和锁B，如此循环。

### std::scoped_lock的实现

我自己在阅读了std::scoped_lock源代码之后，模仿其思路，用C++17语法重新实现了个ScopedLock，以下是一些实现的模板技巧。

#### 变长模板 和 模板特化

ScopedLock使用变长模板来表示多个不同类型的mutex，内部用tuple来存储多个mutex的引用，另外，还提供了两个特化的版本来处理一个参数和无参数的情况：

```cpp
// scoped_lock.hpp
template <typename... MutexType>
class ScopedLock {
 private:
  std::tuple<MutexType&...> mutexs_;
};

template <typename MutexType>
class ScopedLock<MutexType> {

};

template <>
class ScopedLock<> {
};
```

#### 表达式折叠

ScopedLock在析构函数中需要对每个mutex解锁，可以用表达式折叠来展开：(ms.unlock(), ...); std::scoped_lock的做法是使用初始化列表，然后在无用的数组变量上加上__attribute__((__unused__))来避免警告。

```cpp
~ScopedLock() {
   std::apply(
       [](MutexType&... ms) {
         {
           (ms.unlock(), ...);
           // std::scoped_lock
           // char __i[] __attribute__((__unused__)) = { (__m.unlock(), 0)... };
         }
       },
       mutexs_);
 }
```

#### constexper if

Lock函数在ScopedLock的构造函数里面被调用，实现之前描述的死锁避免算法，是ScopedLock的核心部分。首先对Lock1上锁， 当获得Lock1上的锁后，如果Lock函数当前参数个数大于3时则会调用TryLockImpl中静态方法DoTryLock，否则就尝试对Lock2上锁，这里用constexpr if 来决定编译期间生成的哪个分支的代码。

```cpp
explicit ScopedLock(MutexType&... ms) : mutexs_(std::tie(ms...)) {
  // std::lock(ms...);
  Lock(ms...);
}

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
```

TryLockImpl根据索引号Index来获取tuple里面的的mutex，由于是在编译期间完成，所以需要该Index需要是编译期的常量，于是放置在模板参数中：

```cpp
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
```

TryLockImpl中的静态模板函数DoTryLock，首先尝试对第index个mutex上锁，如果失败则直接返回，此时index变量代表对第index个mutex上锁失败；如果上锁成功则继续递归，这里用if constexpr来选择是否继续递归。 递归成功的终止条件是：最后一个mutex上锁成功，此时变量index被赋值为-1，表示完全n个mutex都上锁成功。


