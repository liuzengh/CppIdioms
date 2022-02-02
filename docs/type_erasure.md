## C++编程技巧：Type Erasure

Type Erasure 是一种对不同类型使用单一的运行时表示，但只会根据各自声明的类型使用它们的编程技术。


> Variants of the technique of using a single run-time representation for values of a number of
types and relying on the (static) type system to ensure that they are used only according to their
declared type has been called *type erasure* - Bjarne Stroustrup `<<The C++ Programming Language 4ed>>`

也就是说，Type Erasure对不同类型进行了高级抽象，实现了某种统一表示，但是又保留了实际类型的行为和数据。我们知道C++、Java都是强类型语言，所有的变量在声明的时候一旦被指定成了某个具体的数据类型，如果不经过强制转换就永远就是这种类型了。与强类型相对应的是类似于Python这样的弱类型语言，允许不同类型之间进行自由转换。不过我们可以用C++17引入的std::any来改变变量的内部类型，且保证仍然是类型安全的：

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

先占个坑，下回补齐.

### 参考

Chapter17 std::any  Nicolai M. Josuttis `C++17 the Complete Guide`
Chapter25 Specialization. Bjarne Stroustrup  `<<The C++ Programming Language 4ed>>`
[stackoverflow: Type erasure techniques](https://stackoverflow.com/questions/5450159/type-erasure-techniques)