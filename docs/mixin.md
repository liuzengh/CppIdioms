## Mixin

Mixin(**M**ix in) 是一种将若干功能独立的类通过继承的方式实现模块复用的C++模板编程技巧。其基本做法是将模板参数作为派生类的基类:

```c++
template<typename... Mixins>
class MixinClass : public Mixins... {
  public:
    MixinClass() :  Mixins...() {}
  // ...
};
```

Mixin本身是面向对象领域的一个非常宽泛的概念，它是有一系列被称为Mixin的类型，这些类型分别实现一个单独的功能，且这些功能本身是正交的。当需要使用这些功能时，就可以将不同的mixin组合在一起，像搭积木一样，完成功能复用。

以对二维上的点建模为例，一个Point类可以由多种不相关的属性构成，如二维坐标(x, y), 颜色(color)和标签(label)。为此，我们有多种实现方式：

1. 把坐标，颜色，标签都放在一起，只定义一个Point类即可。这样代码就写死了，如果需要再增加点的透明度属性，就必须修改Point类，扩展性差；如果需要对坐标，颜色，标签做某种操作都需要在Point类中进行改动，复杂性太高。
2. 定义一个Point类作为父类，然后使用继承派生出 LabeledPoint， ColorPoint，LabeledColorPoint类，很明显当需要扩展功能时，会产生很多类。
3. 定义坐标、颜色、标签不同类，然后将其组合起来形成一个Point类。Mixin技巧的的思路和这个一致的，不过使用了模板能更大程度上实现复用。

```cpp
// point_example.cpp

template <typename... Mixins>
class Point : public Mixins... {
 public:
  double x, y;
  Point() : Mixins()..., x(0.0), y(0.0) {}
  Point(double x, double y) : Mixins()..., x(x), y(y) {}
};

class Label {
 public:
  std::string label;
  Label() : label("") {}
};

class Color {
 public:
  unsigned char red = 0, green = 0, blue = 0;
};

using MyPoint = Point<Label, Color>;
```

### std::nested_exception

C++11引入了可嵌套的异常类std::nested_exception，该类其实是Mixin类，可以捕获和存储当前的异常，从而将任意类型的异常嵌套在一起。

1. 定义可嵌套异常类std::nested_exception，这个是对库使用者展示的类。内部就定义了一个exception_ptr类型的对象_M_ptr来存放异常，异常在构造函数中由current_exception()获取当前异常进行初始化:

```cpp
class nested_exception {
  exception_ptr _M_ptr;
 public:
  nested_exception() noexcept : _M_ptr(current_exception()) {}
  virtual ~nested_exception() noexcept;
  exception_ptr nested_ptr() const noexcept { return _M_ptr; }
  // ...
};
```

2. 使用Mixin技巧，实现内部_Nested_exception。_Nested_exception继承自模板参数_Except（一般异常）和 nested_exception:

```cpp
template<typename _Except>
  struct _Nested_exception : public _Except, public nested_exception
  {
    explicit _Nested_exception(const _Except& __ex)
    : _Except(__ex)
    { }
    explicit _Nested_exception(_Except&& __ex)
    : _Except(static_cast<_Except&&>(__ex))
    { }
  };
```

3. 抛出嵌套异常 throw_with_nested。throw_with_nested会根据异常是否从std::nested_exception中派生而来选择不同的实现`__throw_with_nested_impl(true_type)`或者`__throw_with_nested_impl(false_type)`:

```cpp
template <typename _Tp>
inline void __throw_with_nested_impl(_Tp&& __t, true_type) {
  using _Up = typename remove_reference<_Tp>::type;
  throw _Nested_exception<_Up>{std::forward<_Tp>(__t)};
}

template <typename _Tp>
inline void __throw_with_nested_impl(_Tp&& __t, false_type) {
  throw std::forward<_Tp>(__t);
}

/// If @p __t is derived from nested_exception, throws @p __t.
/// Else, throws an implementation-defined object derived from both.
template <typename _Tp>
[[noreturn]] inline void throw_with_nested(_Tp&& __t) {
  using _Up = typename decay<_Tp>::type;
  using _CopyConstructible =
      __and_<is_copy_constructible<_Up>, is_move_constructible<_Up>>;
  static_assert(_CopyConstructible::value,
                "throw_with_nested argument must be CopyConstructible");
  using __nest = __and_<is_class<_Up>, __bool_constant<!__is_final(_Up)>,
                        __not_<is_base_of<nested_exception, _Up>>>;
  std::__throw_with_nested_impl(std::forward<_Tp>(__t), __nest{});
}
```

### 实现可跟踪的异常

有时我们需要知道在代码哪个地方（某个文件某个函数的某行）抛出了异常，实现可跟踪的异常，这和编译错误时的信息提示类似。然而std::exception并没有提供该功能，当然我们可以从std::exception继承实现TraceableException，但是现在代码库里面已经有一个功能独立的跟踪类Trace了：

```cpp
class Trace {
 public:
  Trace(std::string file, int line, std::string func)
      : file_(file), line_(line), func_(func){};
  std::string GetStackTrace() const {
    return file_ + ":" + std::to_string(line_) + ":" + func_;
  };

 private:
  std::string file_;
  int line_;
  std::string func_;
};
```

所以我们最终决定使用Mixin技巧，实现一个可跟踪的异常类：

```cpp
template <class Exception>
struct Traceable : public Exception, public Trace {
  Traceable(Exception e, std::string file, int line, std::string func)
      : Exception(std::move(e)), Trace(file, line, func) {}
};
```

使用之前还需要再包装一下：

```cpp
// mp_throw

template <class Exception>
auto MakeTraceable(Exception e, std::string file, int line, std::string func) {
  return Traceable<Exception>(std::move(e), file, line, func);
}

#define MP_THROW(e) \
  throw MakeTraceable((e), __FILE__, __LINE__, __FUNCTION__)
```

如此就能在其他代码中使用我们的可追踪异常类了，可以打印出异常发生的具体位置：

```cpp
// mp_throw_test.cpp

void Foo() { MP_THROW(std::logic_error("oops")); }

int main() {
  try {
    Foo();
  } catch (const std::exception& e) {
    if (auto t = dynamic_cast<const cpp_idioms::Trace*>(&e)) {
      std::cout << "Exception-" << e.what()
                << " happend at: " << t->GetStackTrace() << "\n";
    }
  }
  return 0;
}
```

### 参考


1. [CppCon 2016: Arthur O'Dwyer “Template Normal Programming](https://www.youtube.com/watch?v=vwrXHznaYLA)
2. [stackoverflow: What are Mixins (as a concept)](https://stackoverflow.com/questions/18773367/what-are-mixins-as-a-concept)
3. [cppreference: std::nested_exception](https://en.cppreference.com/w/cpp/error/nested_exception)
4. [阿里云开发者社区：C++继承和组合——带你读懂接口和mixin，实现多功能自由组合](https://developer.aliyun.com/article/583362)
5.  David Vandevoorde, Nicolai M. Josuttis, Douglas Gregor `<<C++ Templates The Complete Guide 2ed>>` Chapter21:Templates and Inheritance 


