## CRTP

CRTP(**C**uriously **R**ecurring **T**emplate **P**attern)，是一种实现静态多态的C++模板编程技巧。其基本做法是将派生类作为模板参数传递给它自己的基类：

```cpp
// CRTP idioms
template <class Derived>
class Base {
    // 可以使用模板参数Derived访问派生类中的成员
};

class Derived : public Base<Derived> {
    // ...
};

template<typename T>
class DerivedTemplate : public Base<DerivedTemplate<T>> {
   // ...
};
```

在上面的代码中，基类Base是一个模板类，派生类Derived继承自基类，继承时派生类把自己作为模板参数传递给了基类。

### 实现静态多态

CRTP可以用来实现静态多态， 其做法是在基类公开接口，在派生类内实现该接口:

```cpp
// static_poly.cpp

template <class Derived> 
struct Base
{
    void Interface() {
        // ...
        static_cast<Derived*>(this)->Implementation();
        // ...
    }
};

struct Derived : Base<Derived> {
    void Implementation();
};
```

在基类中，我们使用static_cast将基类Base转换为传递过来的派生类Derived，然后调用该派生类中的方法。

### 在基类中自定义派生类的行为

CRTP将派生类以模板形参向上传递给基类，使得基类可以在不需要使用虚函数的情况下自定义派生类的行为。例如现在我们打算实现一个对象计数器ObjectCounter，来统计当前一共存在多少个某种类型（例如类型X）的对象：

```cpp
X x1;
// 类型为X的对象存在1个。
{
  X x2;
  // 类型为X的对象存在2个。
}
auto x3 = std::move(x1)
// 类型为X的对象一共存在1个。
```

最直观的方法是定义一个static数据成员count，在对象构造时加1，析构时减1。不过常见的做法存在以下问题：

1. 需要统计的类型都得重构。例如需要统计类型X，Y，Z的对象个数，则需要改动每个类的构造函数、析构函数中的代码，而这种改动的逻辑都是一样的，显得冗余。

2. 如果使用普通继承，把改动基类对应的代码，则只能统计到所有派生类当前创建的对象个数，无法统计到每个单独的派生类创建的对象个数。

很明显，我们这里需要使用继承把公共的代码逻辑放置在基类，以减少代码冗余；另外需要避免只用一种基类笼统计算的情况。假如每个派生类对于一个基类，则可以实现分开统计的目的。我们可以使用CRTP技巧将基类实现为模板类，把派生类作为模板实参传递给基类，这样不同的派生类就对应着不同的基类:

```cpp
// object_counter.hpp

template <class Derived>
class ObjectCounter {
 private:
  inline static std::size_t count{0};

 protected:
  ObjectCounter() { ++count; }
  ObjectCounter(const ObjectCounter&) { ++count; }
  ObjectCounter(ObjectCounter&&) { ++count; }
  ~ObjectCounter() { --count; }

 public:
  static std::size_t CountLive() { return count; }
};

// template <typename T>
// class X : public ObjectCounter<X<T>> {};

// class Y : public ObjectCounter<Y> {};
```

使用刚才实现的ObjectCounter，我们可以统计MyVector类和MyCharString类的对象个数：

```c++
// object_counter_test.cpp

#include "object_counter.hpp"
#include <iostream>

template <typename T>
class MyVector : public cpp_idioms::ObjectCounter<MyVector<T>> {};

class MyCharString : public cpp_idioms::ObjectCounter<MyCharString> {};

int main() {
  MyVector<int> v1, v2;
  MyCharString s1;

  std::cout << "number of MyVector<int>: " << MyVector<int>::CountLive()
            << "\n";

  std::cout << "number of MyCharString: " << MyCharString::CountLive() << "\n";
  return 0;
}
```

### 实现侵入式双链表

我一开始是在阅读brpc源码中的侵入式双链表 butil::LinkedList(butil/containers/linked_list.h)了解到CRTP技巧的。双链表的实现可以分为侵入式和非侵入式，像C风格的需要自己包含prev/next指针的是侵入式链表，侵入式链表广泛用于Linux内核的实现之中；而std::list实现的是非侵入式链表（STL中的容器都使用的是非侵入式容器）。由于非侵入容器使用的是值语义，添加数据的时候至少多了一次拷贝过程，因此比侵入式容器性能要低下；不过侵入式容器需要自己new出内存，不使用时自己要记得释放。

brpc实现侵入式链表的比std::list性能好主要体现在两方面：

1. 插入操作不需要在堆上分配内存
2. 删除单个元素的时间复杂度为O(1),则std::list的复杂度为O(n)

使用CRTP手法实现和应用butil::LinkedList的步骤如下：


1. 定义一个`LinkedList<T>` 容器类来存放链表中的节点`LinkNode<T>`：

```cpp
// linked_list.hpp
template <typename T>
class LinkedList {
 public:
  // The "root" node is self-referential, and forms the basis of a circular
  // list (root_.Next() will point back to the start of the list,
  // and root_->Previous() wraps around to the end of the list).
  LinkedList() {}
  LinkedList(const LinkedList&) = delete;
  LinkedList& operator=(LinkedList&) = delete;
  void Append(LinkNode<T>* e) { e->InsertBefore(&root_); }
  LinkNode<T>* Head() const { return root_.Next(); }
  LinkNode<T>* Tail() const { return root_.Previous(); }
  const LinkNode<T>* End() const { return &root_; }
  bool Empty() const { return Head() == End(); }
 private:
  LinkNode<T> root_;
};
```

2. 定义一个节点类`LinkNode<T>` 来封装各种不同节点的共有行为，相当于CRTP中的基类：

```cpp
// linked_list.hpp

template <typename T>
class LinkNode {
 public:
  // LinkNode are self-referential as default.
  LinkNode() : previous_(this), next_(this) {}
  LinkNode(LinkNode<T>* previous, LinkNode<T>* next)
      : previous_(previous), next_(next) {}
  LinkNode(const LinkNode&) = delete;
  LinkNode& operator=(const LinkNode&) = delete;

  void InsertBefore(LinkNode<T>* e) {
    // ... 
  }
  void InsertBeforeAsList(LinkNode<T>* e) {
    // ...
  }
  void InsertAfter(LinkNode<T>* e) {
    // ...
  }
  void InsertAfterAsList(LinkNode<T>* e) {
    // ...
  }
  void RemoveFromList() {
    // ... 
  }

  LinkNode<T>* Previous() const { return previous_; }
  LinkNode<T>* Next() const { return next_; }

  const T* Value() const { return static_cast<const T*>(this); }
  T* Value() { return static_cast<T*>(this); }

 private:
  LinkNode<T>* previous_;
  LinkNode<T>* next_;
};
```

3. 使用CRTP技巧，自定义节点类派生类，将该派生类作为模板参数传递给基类`LinkNode<T>`:

```cpp
// linked_list_unittest.cpp

class MyNodeType : public LinkNode<MyNoeType> {
  // ...
};
```

4. 可以把自定义的节点类添加在`LinkedList<MyNodeType>`中，进行遍历：

```cpp
// linked_list_unittest.cpp

LinkedList<MyNodeType> list;
LinkNode<MyNodeType>* n1 = ...;
LinkNode<MyNodeType>* n2 = ...;
LinkNode<MyNodeType>* n3 = ...;
list.Append(n1);
list.Append(n3);
n2->InsertBefore(n3);
for (LinkNode<MyNodeType>* node = list.Head();
   node != list.End();
   node = node->Next()) {
  MyNodeType* value = node->Value();
  ...
}
```

### CRPT的其他一些应用

- [enable_shared_from_this](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this)
- [c++20:ranges::view_interface](https://en.cppreference.com/w/cpp/ranges/view_interface)

### 参考
1. David Vandevoorde, Nicolai M. Josuttis, Douglas Gregor `<<C++ Templates The Complete Guide 2ed>>` Chapter18:The Polymorphic Power of Templates, Chapter21:Templates and Inheritance 
2. [cppreference:crtp](https://en.cppreference.com/w/cpp/language/crtp)
3. [wikipedia:Curiously recurring template pattern](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)

