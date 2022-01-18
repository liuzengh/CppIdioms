### C++编程技巧: Pimpl

Pimpl(**P**ointer to **impl**ementation) 是一种减少代码依赖和编译时间的C++编程技巧，其基本思想是将一个外部可见类(visible class)的实现细节（一般是所有私有的非虚成员）放在一个单独的实现类(implementation class)中，而在可见类中通过一个私有指针来间接访问该实现类。下面的几行代码展示了该技巧的具体做法：

```cpp
// 使用Pimpl

// 在头文件person.hpp中
#include <memory>
class Person {
 public:
  Person();
 private:
  // Person类的实现细节放置在该前向声明的实现类中。
  struct Impl;
  // 指向实现类Impl的私有指针
  std::unique_ptr<Impl> pimpl_;
};

// 在源文件person.cpp中
#include "person.hpp"
#include "basic_info.hpp"
#include <string>
#include <memory>
struct Person::Impl {
  std::string name;
  std::string id;
  BasicInfo basic_info;
};
Person::Person() : pimpl_(std::make_unique<Impl>()) {}
```

我们把Peson类的实现细节放在Person::Impl中， 而在Person中使用私有的std::unique_ptr来访问Person::Impl。

#### Pimpl为什么能减少代码依赖和编译时间

如果不使用Pimpl技巧来实现Person类的话，我们只需要把Person::Impl内的所有实现都放置在Person类中：

```cpp
// 不使用Pimpl

// 在头文件person.hpp中
// 需要包含额外的头文件<string> 和 "basic_info.hpp"
#include "basic_info.hpp"
#include <string>
class Person {
 private:
  std::string name;
  std::string id;
  // 需要经常变动的个人信息类BasicInfo
  BasicInfo basic_info;
};
```

这种做法有两个弊端，一是包含进来的<string> 和 "basic_info.hpp"头文件会增加Person类的编译时间；二是Person类依赖于这些头文件，当这些头文件发生改变时Person类必须重新被编译，例如这里的个人信息类BasicInfo就有可能需要频繁变更。

#### 在源文件中定义特殊的成员函数，而不是在头文件由编译器自动合成。

定义Person类时，我们需要显式或隐式地定义5个特殊成员函数:拷贝构造函数、拷贝赋值操作符、移动构造函数、移动赋值操作符和析构函数来确定复制、移动、赋值和销毁Person类的对象时会发生什么。如果我们使用std::unique_ptr来实现Pimpl，这个5个特殊函数都需要在源文件中定义，而不是由编译器在头文件中自动合成：

```c++
// 在头文件person.hpp中
class Person {
 public:
  // 声明5个特殊的函数，而不是由编译器在头文件自动合成
  ~Person();
  Person(Person&& rhs);
  Person& operator=(Person&& rhs);
  Person(const Person& rhs);
  Person& operator=(const Person& rhs);
}

// 在源文件person.cpp中
// 定义5个特殊的成员函数
Person::~Person() = default;
Person::Person(Person&& rhs) = default;
Person& Person::operator=(Person&& rhs) = default;
Person::Person(const Person& rhs)
    : pimpl_{std::make_unique<Impl>(*rhs.pimpl_)} {}
Person& Person::operator=(const Person& rhs) {
  *pimpl_ = *rhs.pimpl_;
  return *this;
};
```

因为头文件"person.hpp"中只有Person::Impl的声明，没有实现，编译时Person::Impl是一个不完整的类型，所以如果编译器在头文件中自动合成的特殊函数需要进行类型完整性检查则会导致编译失败！下面是编译器自动生成的特殊函数的行为：

- 析构函数: std::unique_ptr<Person::Impl>使用默认的deleter，调用delete之前，会用static_assert 在编译阶段对Person::Impl进行类型完整性检查，确保内部裸指针不会指向一个不完整的类型。
- 移动赋值操作符：在赋值之前，需要销毁赋值操作符左边的Person对象，而销毁时需要对Person::Impl进行类型完整性检查。
- 移动构造函数：编译器通常会在出现异常时生成销毁Person对象，而销毁时需要对Person::Impl进行类型完整性检查。
- 拷贝构造函数和拷贝赋值操作符：默认产生的是浅拷贝, 只拷贝了std::unique_ptr；而我们可能需要深拷贝，拷贝指针指向的内容。

编译器自动生成的析构函数、移动赋值操作符和移动构造函数需要对Person::Impl进行类型完整性检查，所以应该放置在源文件中；而编译器自动生成的拷贝构造函数和拷贝赋值操作符采用的是浅拷贝，如果要实现深拷贝也应该在源文件中定义。

#### 参考

1. Herb Sutterd的博客：[GotW #100: Compilation Firewalls](https://herbsutter.com/gotw/_100/)
2. Scott Meyers的 <<Effective Modern C++>> Item 22:When using the Pimpl Idiom, define special
member functions in the implementation file

