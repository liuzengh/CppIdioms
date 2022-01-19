## CppIdioms

### 规范

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++17](https://www.cppstd17.com/)
- [Bazel编译工具](https://bazel.build/)

### Pimpl

Pimpl(**P**ointer to **impl**ementation) 是一种减少代码依赖和编译时间的C++编程技巧，其基本思想是将一个外部可见类(visible class)的实现细节（一般是所有私有的非虚成员）放在一个单独的实现类(implementation class)中，而在可见类中通过一个私有指针来间接访问该实现类。

别名：编译防火墙

相关术语：handle/body, 桥接模式

#### EBCO

EBCO(**E**mpty **B**ase **C**lass **O**ptimization)，是一种优化空基类存储的C++编程技巧。虽然空类大小不为0，但是当空类作为基类时，只要它分配的地址空间和同一类型的其他对象（或子对象）不同，就不需要在派生类中为它分配空间。

别名：EBO

应用：[boost::compressed_pair](https://www.boost.org/doc/libs/1_47_0/libs/utility/compressed_pair.htm), std::vector和std::shared_ptr的allocator member