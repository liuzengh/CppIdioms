### C++编程技巧: EBCO

EBCO(**E**mpty **B**ase **C**lass **O**ptimization)，是一种优化空基类存储的C++编程技巧。EBCO最典型的应用是boost库中的[compressed_pair](https://www.boost.org/doc/libs/1_47_0/libs/utility/compressed_pair.htm)，boost::compressed_pair 和 std::pair非常类似，只不过当其任意一个模板参数是空类时，将使用EBCO来压缩pair的大小。除此之外，EBCO通常被用于能够感知内存分配标准库类，如std::vector和std::shared_ptr都使用了和boost::compressed_pair类似的EBCO技巧来优化其alloctor成员的存储空间。

#### 空类的大小

C++中的空类在运行时，其内部表示不占用任何内存，这意味空类不能包含非静态数据成员、虚函数和虚基类。我们可能会认为空类对象的大小为0，但其实大小为0的类会出现很多弊端，例如,无法在空类对象上使用地址算术运算：

```cpp
ZeroSizeType z[10];
// 计算地址之间的距离
&z[i] - &z[j];
```

所以C++规定任何对象（包括空类对象）都至少需要占有一个字节大小。例如运行下面的代码：

```cpp
// empty_class.cpp
#include <iostream>
class EmptyClass {};
int main() {
  std::cout << "sizeof(EmptyClass): " << sizeof(EmptyClass) << "\n";
  return 0;
}
```

在我的机器上显示EmptyClass的大小为1，不过在内存对齐更严格的机器上可能会打印出另外一个小的整数(通常是4)。

#### 继承体系下空类的大小

EBCO原则：虽然空类大小不为0，不过当空类作为基类时，只要它分配的地址空间和同一类型的其他对象（或子对象）不同，就不需要在派生类中为它分配空间。

> However, even though there are no zero-size types in C++, the C++ standard does specify that when an empty class is used as a base class, no space needs to be allocated for it provided that it does not cause it to be allocated to the same address as another object or subobject of the same type.

例如下面的代码：

```cpp
// ebco_success.cpp
#include <iostream>

class EmptyOne {};
class EmptyTwo : public EmptyOne {};
class EmptyThree : public EmptyTwo {};

int main() {
  std::cout << "sizeof(Empty): " << sizeof(EmptyOne) << '\n';
  std::cout << "sizeof(EmptyTwo): " << sizeof(EmptyTwo) << '\n';
  std::cout << "sizeof(EmptyThree): " << sizeof(EmptyThree) << '\n';
  return 0;
}
```

如果编译器支持EBCO的话，EmptyOne，EmptyTwo和EmptyThree这三个类的大小都是一样的，我机器上显示都是1字节。其内存布局如下：

```
// 编译器支持EBCO
[    ] } EmptyOne } EmptyTwo } EmptyThree
```

若是不支持EBCO则三个类的大小不相等，其内存布局如下：

```
// 编译器不支持EBCO
[    ] } EmptyOne } EmptyTwo } EmptyThree
[    ]            }          }
[    ]                       }
```

##### 不满足EBCO原则的空类继承

当空类作为基类时，如果使用EBCO原则，会导致它分配的地址空间和同一类型的其他对象（或子对象）相同，则EBCO失败，需要在派生类中为它分配空间。例如下面的代码：

```cpp
// ebco_fail.cpp
#include <iostream>

class EmptyOne {};
class EmptyTwo : public EmptyOne {};
class NonEmpty : public EmptyOne, public EmptyTwo {};

int main() {
  std::cout << "sizeof(Empty): " << sizeof(EmptyOne) << '\n';
  std::cout << "sizeof(EmptyTwo): " << sizeof(EmptyTwo) << '\n';
  std::cout << "sizeof(EmptyThree): " << sizeof(NonEmpty) << '\n';
  return 0;
}
```

如果在NonEmpty上使用EBCO，则NonEmpty的父类EmptyOne，和父类EmptyTwo上的EmptyOne将会被分配在同一内存空间。此时EBCO失败，需要为EmptyOne分配空间：

```cpp
[    ] } EmptyOne            } NonEmpty
[    ] } EmptyOne } EmptyTwo }
```

在我测试的机器上NonEmpty的大小为2字节。

##### [[no_unique_address]] 属性显式优化空类大小

C++20引入了[[no_unique_address]]属性，保证相同类型的不同对象的地址总是不同的。例如下面的代码中在X的成员变量empty上使用了[[no_unique_address]]属性，可以直接优化掉空类的大小。

```cpp
// 空类
struct Empty {}; 
 
struct X {
    int i;
    [[no_unique_address]] Empty empty;
};
 
int main() {
    // 空类被优化掉了
    static_assert(sizeof(X) == sizeof(int));
    return 0;
}
```

#### 使用EBCO技巧来优化Tuple

从存储对象的类型是否一致来说，一般可以把c++的容器结构分为两种：

1. 同质结构：如std::array，std::vector，std::list，std::map
2. 异质结构：class，struct，std::tuple

其中的tuple提供了按位置访问元素和从类型列表轻松构造元组的能力，使得其比struct更适合用于模板元编程, 是具有大量潜在用途的基本异构容器。这里打算实现一个简单版本的Tuple，然后使用EBCO技巧对其进行存储优化。

##### Tuple的递归表示

可以使用递归来表达包含n个不同元素的Tuple<T1, T2, ...., Tn>：

1. 基本情况：n = 0 时，Tuple<>
2. 递归关系：n > 0 时，可以把Tuple拆分为两部分，第一部分只包含第一个元素，第二部分包含其余的n-1个元素

```c++ 
Tuple<T1, T2, ...., Tn> = Tuple<T1, Tuple<T2, T3..., Tn>> 
                        = Tuple<T1, Tuple<S1, S2..., Sn-1>>
                        = Tuple<Head, Tail>
其中，Head = T1, Tail = Tuple<S1, S2..., Sn-1>。
```

用变长模板实现如下：

```cpp
// tuple.hpp
template <typename... Types>
class Tuple;

// 基本情况:
template <>
class Tuple<> {
  // 空类, 其实不需要存储空间
};

// 递归关系:
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
 public:
  Tuple() = default;
  Tuple(const Head& head, const Tuple<Tail...>& tail)
      : head_(head), tail_(tail) {}

  Head& GetHead() { return head_; }
  const Head& GetHead() const { return head_; }

  Tuple<Tail...>& GetTail() { return tail_; }
  const Tuple<Tail...>& GetTail() const { return tail_; }

 private:
  Head head_;
  Tuple<Tail...> tail_;
};
```

注意到Tuple有两个地方可能出现空类：

1. 递归形式里面的基本情况 Tuple<>
2. Tuple<T1, T2, ... Tn>的模板参数 T1, T2, ...中可能出现空类

下面我们来一步步尝试用EBCO技巧优化空类的存储空间。

##### 尝试使用EBCO

第1步尝试：就是根据EBCO的使用条件，需要发生继承关系，出现空基类。于是把Tail部分从成员变量变成基类进行继承：

```cpp
template<typename Head, typename... Tail>
class Tuple<Head, Tail...> : private Tuple<Tail...> {
 private:
  Head head_;
 public:
  Head& GetHead() { return head_; }
  Head const& GetHead() const { return head_; }
  Tuple<Tail...>& GetTail() { return *this; }
  Tuple<Tail...> const& GetTail() const { return *this; }
};
```

不过直接继承自Tail很明显会出现初始化顺序相反的问题，因为在C++中需要先构造父类，再构造派生类。

第2步尝试：将Head成员也放入基类列表中位于Tail之前。为了避免直接从trivial 类型中继承，需要将Head成员包装成TupleElement：

```cpp
template <typename T>
class TupleElement {
  T value_;

 public:
  TupleElement() = default;
template<typename U>
TupleElement(U&& other) : value_(std::forward<U>(other) { }
T& Get() {
    return value_; }
T const& Get() const {
    return value_; }
};
```

则可以这样实现Tuple：

```cpp
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> : private TupleElement<Head>,
                             private Tuple<Tail...> {
 public:
  Head& GetHead() {
    // 派生类向基类转换可能出现歧义
    return static_cast<TupleElement<Head>*>(this)->Get();
  }
  Head const& GetHead() const {
    // 派生类向基类转换可能出现歧义
    return static_cast<TupleElement<Head> const*>(this)->Get();
  }
  Tuple<Tail...>& GetTail() { return *this; }
  Tuple<Tail...> const& GetTail() const { return *this; }
};
```

不过这种实现方式引入一个更糟的问题:不能从具有两个相同类型元素的tuple中提取元素，例如Tuple<int, int>， 因为从该类型的派生类Tuple到基类TupleElement(例如TupleElt<int>)的转换具有歧义。

第3步尝试：为了打破歧义，我们需要确保每个TupleElement基类在给定的Tuple中是唯一的。 一种方法是将Tail的所在的长度作为Height，编码到TupleElement中：

```cpp
// tuple_element1.hpp
template <unsigned int Height, typename T>
class TupleElement {
 private:
  T value_;

 public:
  TupleElement() = default;
  template <typename U>
  TupleElement(U&& other) : value_(std::forward<U>(other)){};
  T& Get() { return value_; }
  const T& Get() const { return value_; }
};
```

然后重新改写Tuple：

```cpp
// tuple_storage1.hpp
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> : private TupleElement<sizeof...(Tail), Head>,
                             private Tuple<Tail...> {
  using HeadElt = TupleElement<sizeof...(Tail), Head>;

 public:
  Head& GetHead() { return static_cast<HeadElt*>(this)->Get(); }
  Head const& GetHead() const {
    return static_cast<HeadElt const*>(this)->Get();
  }
  Tuple<Tail...>& GetTail() { return *this; }
  Tuple<Tail...> const& GetTail() const { return *this; }
};
```

至此，已经将递归基本情况里的Tuple<>存储空间给优化掉了。不过我们还可以更进一步，将Tuple<T1, T2, ... Tn>模板参数中可能出现空类给优化掉。

第4步尝试：优化Tuple<T1, T2, ... Tn>的模板参数中可能出现空类。只需要在第3的基础上稍作修改，使得TupleElement可以从模板参数中继承即可：

```cpp
// tuple_element2.hpp
template <unsigned Height, typename T,
          bool = std::is_class_v<T> && !std::is_final_v<T>>
class TupleElement;

template <unsigned Height, typename T>
class TupleElement<Height, T, false> {
  T value;

 public:
  TupleElement() = default;
  template <typename U>
  TupleElement(U&& other) : value(std::forward<U>(other)) {}
  T& get() { return value; }
  T const& get() const { return value; }
};

template <unsigned Height, typename T>
class TupleElement<Height, T, true> : private T {
 public:
  TupleElement() = default;
  template <typename U>
  TupleElement(U&& other) : T(std::forward<U>(other)) {}
  T& get() { return *this; }
  T const& get() const { return *this; }
};
```

最后写一段程序来验证实现的正确性：

```c++
// compressed_tuple2.cpp
#include <iostream>
#include "tuple_storage2.hpp"

struct A {
  A() { std::cout << "A()" << '\n'; }
};

struct B {
  B() { std::cout << "B()" << '\n'; }
};

int main() {
  cpp_idioms::Tuple<A, char, A, char, B> t1;
  std::cout << sizeof(t1) << " bytes" << '\n';
  return 0;
}
```

A、B都是空类，使用最后优化过的Tuple，显示t1的大小为2字节。

#### 参考
1. David Vandevoorde, Nicolai M. Josuttis, Douglas Gregor <<C++ Templates The Complete Guide Second Edition>> Chapter21: Templates and Inheritance && Chapter25 Tuple
2. [cppreference.com : Empty base optimization](https://en.cppreference.com/w/cpp/language/ebo) 
3. [<<More C++ Idioms>> :Empty Base Optimization](https://en.m.wikibooks.org/wiki/More_C%2B%2B_Idioms/Empty_Base_Optimization)
4. Stanley B. Lippman <<Inside the C++ Object Model>> Chapter3: The Semantics of Data