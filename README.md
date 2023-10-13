## CppIdioms

### 规范

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++17](https://www.cppstd17.com/)
- [Bazel编译工具](https://bazel.build/)
- [GoogleTest](https://github.com/google/googletest)

### Pimpl

Pimpl(**P**ointer to **impl**ementation) 是一种减少代码依赖和编译时间的C++编程技巧，其基本思想是将一个外部可见类(visible class)的实现细节（一般是所有私有的非虚成员）放在一个单独的实现类(implementation class)中，而在可见类中通过一个私有指针来间接访问该实现类。

别名：编译防火墙

相关术语：handle/body, 桥接模式

#### EBCO

EBCO(**E**mpty **B**ase **C**lass **O**ptimization)，是一种优化空基类存储的C++编程技巧。虽然空类大小不为0，但是当空类作为基类时，只要它分配的地址空间和同一类型的其他对象（或子对象）不同，就不需要在派生类中为它分配空间。

别名：EBO

应用：[boost::compressed_pair](https://www.boost.org/doc/libs/1_47_0/libs/utility/compressed_pair.htm), std::vector和std::shared_ptr的allocator member

#### CRTP

CRTP(**C**uriously **R**ecurring **T**emplate **P**attern)，是一种实现静态多态的C++模板编程技巧。其基本做法是将派生类作为模板参数传递给它自己的基类：

```cpp
// CRTP idioms
template <class Derived>
class Base {
};
class Derived : public Base<Derived> {
    // ...
};
template<typename T>
class DerivedTemplate : public Base<DerivedTemplate<T>> {
   // ...
};
```

应用：[侵入式双链表butil::LinkedList](https://github.com/apache/incubator-brpc/blob/master/src/butil/containers/linked_list.h)，[enable_shared_from_this](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this)和[c++20:ranges::view_interface](https://en.cppreference.com/w/cpp/ranges/view_interface)


#### Mixin

Mixin(**M**ix in) 是一种将若干功能独立的类通过继承的方式实现模块复用的C++模板编程技巧。其基本做法是将模板参数作为派生类的基类:

```c++
template<typename... Mixins>
class MixinClass : public Mixins... {
  public:
    MixinClass() :  Mixins()... {}
  // ...
};
```

应用：[std::nested_exception](https://en.cppreference.com/w/cpp/error/nested_exception)

#### Scoped Locking

Scoped Locking 是将RAII手法应用于locking的并发编程技巧。其具体做法就是在构造时获得锁，在析构时释放锁，目前Scoped Locking技巧在C++中有以下4种实现：

- std::lock_guard (c++11): 单个std::mutex(或std::shared_mutex) 
- std::unique_lock (c++11): 单个std::mutex(或std::shared_mutex), 用法比std::lock_guard更灵活
- std::shared_lock (c++14): 单个std::shared_mutex
- std::scoped_lock (c++17): 多个std::mutex(或std::shared_mutex)

应用：std::lock_guard, std::unique_lock, std::shared_lock 和 std::scoped_lock


#### Type Erasure

Type Erasure 是一种对不同类型使用单一的运行时表示，但只会根据各自声明的类型使用它们的编程技术。


> Variants of the technique of using a single run-time representation for values of a number of
types and relying on the (static) type system to ensure that they are used only according to their
declared type has been called *type erasure* - Bjarne Stroustrup `<<The C++ Programming Language 4ed>>`

也就是说，Type Erasure对不同类型进行了高级抽象，实现了某种统一表示，但是又保留了实际类型的行为和数据。我们知道C++、Java都是强类型语言，所有的变量在声明的时候一旦被指定成了某个具体的数据类型，如果不经过强制转换就永远就是这种类型了。与强类型相对应的是类似于Python这样的弱类型语言，允许不同类型之间进行自由转换。

应用: std::any 和 std::function
