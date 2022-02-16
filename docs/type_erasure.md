## C++编程技巧：Type Erasure

Type Erasure 是一种对不同类型使用单一的运行时表示，但只会根据各自声明的类型使用它们的编程技术。


> Variants of the technique of using a single run-time representation for values of a number of
types and relying on the (static) type system to ensure that they are used only according to their
declared type has been called *type erasure* - Bjarne Stroustrup `<<The C++ Programming Language 4ed>>`

也就是说，Type Erasure对不同类型进行了高级抽象，实现了某种统一表示，但是又保留了实际类型的行为和数据。我们知道C++、Java都是静态类型语言，所有的变量在声明的时候一旦被指定成了某个具体的数据类型，如果不经过强制转换就永远就是这种类型了。与静态类型相对应的是类似于Python这样的动态类型语言，允许不同类型之间进行自由转换。不过我们可以用C++17引入的std::any来改变变量的内部类型，且保证仍然是类型安全的：

```cpp
std::any a;             // a is empty
a = 4.3;                // a has value 4.3 of type double
a = 42;                 // a has value 42 of type int
a = std::string{"hi"};  // a has value "hi" of type std::string
if (a.type() == typeid(std::string)) {
  std::string s = std::any_cast<std::string>(a);
  UseString(s);
} else if (a.type() == typeid(int)) {
  UseInt(std::any_cast<int>(a));
}
```

上述代码在声明变量a为std::any之后，允许动态地改变其内部类型。 std::any可以用来表示任何可拷贝构造的单值类型,对类型的数据进行了抽象。除了对类型的数据进行抽象外，也可以对类型的行为进行抽象，例如std::function可以用来表示所有的可被调用的对象：普通函数、成员函数、函数对象、lambda表达式。

我们不免会想一下std::any和std::function的内部实现，在C语言中可以空指针`void*`来表示类型的任意数据，用函数指针来表示类型的任意行为，最后都是pointer! 实际上C++中的Type Erasure的所有实现都是这样的。下面以Any(std::any)为例子来简要分析下。

### Any的实现

**内部数据表示与小对象优化**

Any中声明了一个类型为Storage的变量storage_来存储实际的类型，而Storage作为一个内部定义的union，要么是一个`void*`的指针，要么是一个大小为`void*`,且与`void*`对齐的一块小内存空间：

```cpp
class Any {
  // Holds either pointer to a heap object or the contained object itself.
  union Storage {
    constexpr Storage() : ptr{nullptr} {}

    // Prevent trivial copies of this type, buffer might hold a non-POD.
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    void* ptr;
    std::aligned_storage_t<sizeof(ptr), alignof(void*)> buffer;
  };
  Storage storage_;
};
```

这样定义的Storage的目的是为了使用小对象优化，当要存储的数据大小和对齐都小于Storage时则使用内部管理方式ManagerInternal存储在buffer中（栈上），否则则使用外部管理ManagerExternal的方式存储在堆上。

```cpp
template <typename Tp, bool Fits = (sizeof(Tp) <= sizeof(Storage)) &&
                                   (alignof(Tp) <= alignof(Storage))>
using Internal =
    std::integral_constant<bool,
                           std::is_nothrow_move_constructible_v<Tp> && Fits>;
template <typename Tp>
struct ManagerInternal;
template <typename Tp>
struct ManagerExternal;
template <typename Tp>
using Manager = std::conditional_t<Internal<Tp>::value, ManagerInternal<Tp>,
                                   ManagerExternal<Tp>>;
```

为了区分是在栈上还是在堆上构造对象，ManagerInternal 和 ManagerExternal的Create函数中分别使用了placement new 和 new，同时能接受变长参数：

```cpp
  // Manage in-place contained object
  template <typename Tp>
  struct ManagerInternal {
    template <typename Up>
    static void Create(Storage& storage, Up&& value) {
      void* addr = &storage.buffer;
      ::new (addr) Tp(std::forward<Up>(value));
    }

    template <typename... Args>
    static void Create(Storage& storage, Args&&... args) {
      void* addr = &storage.buffer;
      ::new (addr) Tp(std::forward<Arg>(args)...);
    }
  };

  // Manage external contained object.
  template <typename Tp>
  struct ManagerExternal {
    template <typename Up>
    static void Create(Storage& storage, Up&& value) {
      storage.ptr = new Tp(std::forward<Up>(value));
    }

    template <typename... Args>
    static void Create(Storage& storage, Args&&... args) {
      storage.ptr = new Tp(std::forward<Args>(args)...);
    }
  };
};
```

**类型行为管理**

Any中使用了一个函数指针manager_来管理内部的类型的行为，如查询、拷贝、销毁和移动：

```cpp
class Any {
void (*manager_)(Operator, const Any*, Arg*);
};
```

其实内部的类型也存储在manager_中，因为构造Any时manager_指针由ManagerInternal或ManagerExternal的静态函数Manage初始化，而ManagerInternal和ManagerExternal都是模板类，变换一种新类型的时候就会根据传递进来的模板实参来生成具体的ManagerInternal和ManagerExternal：

```cpp
class Any {
  template <typename Tp>
  struct ManagerInternal {
    static void Manage(Operator which, const Any* any_ptr, Arg* arg);
  };
  template <typename Tp>
  struct ManagerExternal {
    static void Manage(Operator which, const Any* any_ptr, Arg* arg);
  };
 private:
  enum class Operator { kAccess, KGetTypeInfo, kClone, kDestroy, kXfer };
  union Arg {
    void* obj;
    const std::type_info* typeinfo_ptr;
    Any* any_ptr;
  };
};
```

ManagerInternal和ManagerExternal的Manage函数使用了*策略模式*，会根据不同的Operator采用不同的策略。ManagerInternal和ManagerExternal的Manage函数实现不同的最大原因是一个在堆上，一个在栈上：

```cpp
template <typename Tp>
void Any::ManagerInternal<Tp>::Manage(Operator which, const Any* any,
                                      Arg* arg) {
  // the contained object is in storage_.buffer
  auto ptr = reinterpret_cast<const Tp*>(&any->storage_.buffer);
  switch (which) {
    case Operator::kAccess:
      arg->obj = const_cast<Tp*>(ptr);
      break;
    case Operator::KGetTypeInfo:
      arg->typeinfo_ptr = &typeid(Tp);
      break;
    case Operator::kClone:
      ::new (&arg->any_ptr->storage_.buffer) Tp(*ptr);
      arg->any_ptr->manager_ = any->manager_;
      break;
    case Operator::kDestroy:
      ptr->~Tp();
      break;
    case Operator::kXfer:
      ::new (&arg->any_ptr->storage_.buffer)
          Tp(std::move(*const_cast<Tp*>(ptr)));
      ptr->~Tp();
      arg->any_ptr->manager_ = any->manager_;
      const_cast<Any*>(any)->manager_ = nullptr;
      break;
  }
}

template <typename Tp>
void Any::ManagerExternal<Tp>::Manage(Operator which, const Any* any,
                                      Arg* arg) {
  // the contained object is in storage_.ptr
  auto ptr = static_cast<const Tp*>(any->storage_.ptr);
  switch (which) {
    case Operator::kAccess:
      arg->obj = const_cast<Tp*>(ptr);
      break;
    case Operator::KGetTypeInfo:
      arg->typeinfo_ptr = &typeid(Tp);
      break;
    case Operator::kClone:
      arg->any_ptr->storage_.ptr = new Tp(*ptr);
      arg->any_ptr->manager_ = any->manager_;
      break;
    case Operator::kDestroy:
      delete ptr;
      break;
    case Operator::kXfer:
      arg->any_ptr->storage_.ptr = any->storage_.ptr;
      arg->any_ptr->manager_ = any->manager_;
      const_cast<Any*>(any)->manager_ = nullptr;
      break;
  }
}
```

manager_的行为会在拷贝构造函数、移动构造函数，拷贝赋值，移动赋值，析构函数中体现。

### std::function的实现

std:: function<> 实际上是c++函数指针的通用形式，提供相同的基本操作:

- 可以在调用者不知道函数体的情况下，调用函数
- 可复制、可移动和可赋值
- 能够被其他函数（或兼容形式）初始化或赋值
- 存在空状态来表明当前没有与之绑定的函数

但是，与c++函数指针不同，std::function<> 可以存储lambda表达式或函数对象。

接下来我们将构建通用函数指针类模板FunctionPtr，以提供上述相同的核心操作和功能，并可用于替代std::function（本实现参考自`<<C++ Templates The Complete Guide 2ed>>`一书中的FunctionPtr。）:


```cpp
// function_ptr.hpp

template <typename Signature>
class FunctionPtr;

template <typename R, typename... Args>
class FunctionPtr<R(Args...)> {
 public:
  FunctionPtr() : bridge_{nullptr} {}

  FunctionPtr(const FunctionPtr&);

  FunctionPtr(FunctionPtr& other)
      : FunctionPtr(static_cast<FunctionPtr const&>(other)) {}
  FunctionPtr(FunctionPtr&& other) : bridge_{other.bridge_} {
    other.bridge_ = nullptr;
  }

  template <typename F>
  FunctionPtr(F&& f);

  FunctionPtr& operator=(const FunctionPtr& other) {
    FunctionPtr temp(other);
    swap(*this, temp);
    return *this;
  }

  FunctionPtr& operator=(FunctionPtr&& other) {
    delete bridge_;
    bridge_ = other.bridge_;
    other.bridge_ = nullptr;
    return *this;
  }

  template <typename F>
  FunctionPtr& operator=(F&& f) {
    FunctionPtr temp(std::forward<F>(f));
    swap(*this, temp);
    return *this;
  }

  ~FunctionPtr() { delete bridge_; }

  friend void swap(FunctionPtr& fp1, FunctionPtr& fp2) {
    std::swap(fp1.bridge_, fp2.bridge_);
  }

  explicit operator bool() const { return bridge_ == nullptr; }

  R operator()(Args... args) const;

 private:
  FunctorBridge<R, Args...>* bridge_;
};
```

FunctionPtr包含一个非静态成员变量bridge_，负责函数对象的存储和操作。该指针的所有权绑定到FunctionPtr对象，因此所提供的大多数接口仅仅管理该指针。

#### 动态多态： Bridge Interface

FunctorBridge类模板负责底层函数对象的所有权和操作。它被实现为一个抽象基类，保证了FunctionPtr的动态多态性:

```cpp
// function_bridge.hpp

template <typename R, typename... Args>
class FunctorBridge {
 public:
  virtual ~FunctorBridge() {}
  virtual FunctorBridge* Clone() const = 0;
  virtual R Invoke(Args... arg) const = 0;
  virtual bool Equals(const FunctorBridge* fb) const = 0;
};
```

使用这些虚函数，可以实现FunctionPtr的复制构造函数和函数调用操作符:

```cpp
// function_ptr.hpp
template <typename R, typename... Args>
FunctionPtr<R(Args...)>::FunctionPtr(const FunctionPtr& other)
    : bridge_{nullptr} {
  if (other.bridge_) {
    bridge_ = other.bridge_->Clone();
  }
}

template <typename R, typename... Args>
R FunctionPtr<R(Args...)>::operator()(Args... args) const {
  return bridge_->Invoke(std::forward<Args>(args)...);
}
```

### 静态多态：SpecificFunctorBridge

FunctorBridge的每个实例都是一个抽象类，因此它的派生类负责实现其虚函数。为了支持各种类型的函数对象，我们需要定义许多派生类，不过幸运的是，这里可以通过将派生类存储的函数对象的类型进行参数化来实现这一点：


```cpp
// specific_functor_bridge.hpp
template <typename Functor, typename R, typename... Args>
class SpecificFunctorBridge : public FunctorBridge<R, Args...> {
 public:
  template <typename FunctorFwd>
  SpecificFunctorBridge(FunctorFwd&& functor)
      : functor_(std::forward<FunctorFwd>(functor)) {}

  virtual SpecificFunctorBridge* Clone() const override {
    return new SpecificFunctorBridge(functor_);
  }

  virtual R Invoke(Args... args) const override {
    return functor_(std::forward<Args>(args)...);
  }

 private:
  Functor functor_;
};
```

SpecificFunctorBridge的每个实例都存储了函数对象(其类型为Functor)的一个副本，它可以被调用、复制(通过克隆SpecificFunctorBridge)或销毁(隐式地在析构函数中)。每当FunctionPtr被初始化为一个新的函数对象时，就会创建SpecificFunctorBridge实例:

```cpp
// function_ptr.hpp
template <typename R, typename... Args>
template <typename F>
FunctionPtr<R(Args...)>::FunctionPtr(F&& f) : bridge_{nullptr} {
  using Functor = std::decay_t<F>;
  using Bridge = SpecificFunctorBridge<Functor, R, Args...>;
  bridge_ = new Bridge(std::forward<F>(f));
}
```

需要注意的是，虽然FunctionPtr构造函数本身是在函数对象类型F上模板化的，但该类型只有通过SpecificFunctorBridge(由Bridge类型别名描述)的特定特化才能知道。一旦新分配的Bridge实例被分配给数据成员Bridge，由于从Bridge 到`FunctorBridge<R, Args…>`这种类型信息的丢失解释了为什么Type Erasure通常用于描述静态和动态多态性之间的桥接技术。

Type Erasure提供了静态多态和动态多态的一些优点，但不是全部。特别是，使用Type Erasure生成的代码的性能更接近于动态多态性，因为两者都通过虚拟函数使用动态dispatch。因此，静态多态性的一些传统优势，比如编译器内联调用的能力，可能会丧失。这种性能损失是否可察觉取决于应用程序，但通常很容易通过考虑相对于虚函数调用的代价，在被调用的函数中执行了多少工作来判断:如果两者接近(例如，使用FunctionPtr简单地添加两个整数)，Type Erasure可能比静态多态版本执行得慢得多；如果函数调用执行大量的工作—查询数据库、对容器排序或更新用户界面，则Type Erasure的开销不太可能测量。

### 参考

1. Chapter17 std::any.  Nicolai M. Josuttis `<<C++17 the Complete Guide>>`
2. Chapter22: Bridging static and dynamic Polymorphism.  David Vandevoorde, Nicolai M. Josuttis, Douglas Gregor `<<C++ Templates The Complete Guide 2ed>>`
3. Chapter25 Specialization. Bjarne Stroustrup  `<<The C++ Programming Language 4ed>>`
4. [stackoverflow: Type erasure techniques](https://stackoverflow.com/questions/5450159/type-erasure-techniques)