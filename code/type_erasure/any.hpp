#pragma once

#include <initializer_list>
#include <typeinfo>
#include <utility>

// for aligned_storage
#include <type_traits>

// template <typename E>
// constexpr auto ToUType(E enumerator) noexcept {
//   return static_cast<std::underlying_type_t<E>>(enumerator);
// }

namespace cpp_idioms {

class BadAnyCast : public std::bad_cast {
 public:
  virtual const char* what() const noexcept override { return "bad any_cast"; }
};

inline void ThrowBadAnyCast() { throw BadAnyCast{}; }

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

  template <typename Tp, typename Decayed = std::decay_t<Tp>>
  using Decay = std::enable_if_t<!std::is_same_v<Decayed, Any>, Decayed>;

  // Emplace with an object created from args as the contained object
  template <typename Tp, typename... Args, typename Mgr = Manager<Tp>>
  void DoEmplace(Args&&... args) {
    Mgr::Create(storage_, std::forward<Args>(args)...);
    manager_ = &Mgr::Manage;
  }

  // Emplace with an object created from il and args as the contained object.
  template <typename Tp, typename Up, typename... Args,
            typename Mgr = Manager<Tp>>
  void DoEmplace(std::initializer_list<Up> il, Args&&... args) {
    Reset();
    Mgr::Create(storage_, il, std::forward<Args>(args)...);
    manager_ = &Mgr::Manage;
  }

 public:
  // construct/destruct

  // Default constructor, create an empty object.
  constexpr Any() noexcept : manager_(nullptr) {}

  // Copy constructor, copies the state of other
  Any(const Any& other) {
    if (!other.HasValue()) {
      manager_ = nullptr;
    } else {
      Arg arg;
      arg.any_ptr = this;
      other.manager_(Operator::kClone, &other, &arg);
    }
  }
  // Move constructor, transfer the state from other
  Any(Any&& other) {
    if (!other.HasValue()) {
      manager_ = nullptr;
    } else {
      Arg arg;
      arg.any_ptr = this;
      other.manager_(Operator::kXfer, &other, &arg);
    }
  }

  template <typename Res, typename Tp, typename... Args>
  using any_constructible =
      std::enable_if<std::conjunction_v<std::is_copy_constructible<Tp>,
                                        std::is_constructible<Tp, Args...>>,
                     Res>;

  template <typename Tp, typename... Args>
  using any_constructible_t =
      typename any_constructible<bool, Tp, Args...>::type;

  // Construct with a copy of value as the contained object
  template <typename ValueType, typename Tp = Decay<ValueType>,
            typename Mgr = Manager<Tp>,
            any_constructible_t<Tp, ValueType&&> = true,
            std::enable_if_t<!std::__is_in_place_type<Tp>::value, bool> = true>
  Any(ValueType&& value) : manager_(&Mgr::Manage) {
    Mgr::Create(storage_, std::forward<ValueType>(value));
  }

  // Construct with a copy of value as the contained object
  template <typename ValueType, typename Tp = Decay<ValueType>,
            typename Mgr = Manager<Tp>,
            std::enable_if_t<
                std::conjunction_v<
                    std::is_copy_constructible<Tp>,
                    std::negation<std::is_constructible<Tp, ValueType&&>>,
                    std::negation<std::__is_in_place_type<Tp>>>,
                bool> = false>
  Any(ValueType&& value) : manager_(&Mgr::Manage) {
    Mgr::Create(storage_, value);
  }

  /// Construct with an object created from @p Args as the contained object.
  template <typename ValueType, typename... Args,
            typename Tp = Decay<ValueType>, typename Mgr = Manager<Tp>,
            any_constructible_t<Tp, Args&&...> = false>
  explicit Any(std::in_place_type_t<ValueType>, Args&&... args)
      : manager_(&Mgr::Manage) {
    Mgr::Create(storage_, std::forward<Args>(args)...);
  }

  /// Construct with an object created from @p il and @p Args as
  /// the contained object.
  template <
      typename ValueType, typename Up, typename... Args,
      typename Tp = Decay<ValueType>, typename Mgr = Manager<Tp>,
      any_constructible_t<Tp, std::initializer_list<Up>, Args&&...> = false>
  explicit Any(std::in_place_type_t<ValueType>, std::initializer_list<Up> il,
               Args&&... args)
      : manager_(&Mgr::Manage) {
    Mgr::Create(storage_, il, std::forward<Args>(args)...);
  }

  // Destructor
  ~Any() { Reset(); }

  // assignments

  // Copy the state of another object
  Any& operator=(const Any& rhs) {
    *this = Any(rhs);
    return *this;
  }

  // Move assignment operator
  Any& operator=(Any&& rhs) noexcept {
    if (!rhs.HasValue()) {
      Reset();
    } else if (this != &rhs) {
      Reset();
      Arg arg;
      arg.any_ptr = this;
      rhs.manager_(Operator::kXfer, &rhs, &arg);
    }
    return *this;
  }

  // Store a copy of rhs as the contained object
  template <typename ValueType>
  std::enable_if_t<std::is_copy_constructible_v<Decay<ValueType>>, Any&>
  operator=(ValueType&& rhs) {
    *this = Any(std::forward<ValueType>(rhs));
    return *this;
  }

  /// Emplace with an object created from @p Args as the contained object.
  template <typename ValueType, typename... Args>
  any_constructible_t<Decay<ValueType>&, Decay<ValueType>, Args&&...> emplace(
      Args&&... args) {
    DoEmplace<Decay<ValueType>>(std::forward<Args>(args)...);
    Any::Arg arg;
    this->manager_(Any::Operator::kAccess, this, &arg);
    return *static_cast<Decay<ValueType>*>(arg.obj);
  }

  /// Emplace with an object created from @p il and @p Args as
  /// the contained object.
  template <typename ValueType, typename Up, typename... Args>
  any_constructible_t<Decay<ValueType>&, Decay<ValueType>,
                      std::initializer_list<Up>, Args&&...>
  Emplace(std::initializer_list<Up> il, Args&&... args) {
    DoEmplace<Decay<ValueType>, Up>(il, std::forward<Args>(args)...);
    Any::Arg arg;
    this->manager_(Any::Operator::kAccess, this, &arg);
    return *static_cast<Decay<ValueType>*>(arg.obj);
  }

  // modifiers
  void Reset() noexcept {
    if (HasValue()) {
      manager_(Operator::kDestroy, this, nullptr);
      manager_ = nullptr;
    }
  }
  // Exchange state with another object
  void Swap(Any& rhs) noexcept {
    if (!HasValue() && !rhs.HasValue()) return;
    if (HasValue() && rhs.HasValue()) {
      if (this == &rhs) return;
      Any temp;
      Arg arg;
      arg.any_ptr = &temp;
      rhs.manager_(Operator::kXfer, &rhs, &arg);
      arg.any_ptr = &rhs;
      manager_(Operator::kXfer, this, &arg);
      arg.any_ptr = this;
      temp.manager_(Operator::kXfer, &temp, &arg);

    } else {
      Any* empty = !HasValue() ? this : &rhs;
      Any* full = !HasValue() ? &rhs : this;
      Arg arg;
      arg.any_ptr = empty;
      full->manager_(Operator::kXfer, full, &arg);
    }
  }

  // observers
  bool HasValue() const noexcept { return manager_ != nullptr; }

  // The typeid of the contained object, or typeid(void) if empty
  const std::type_info& Type() const noexcept {
    if (!HasValue()) return typeid(void);
    Arg arg;
    manager_(Operator::KGetTypeInfo, this, &arg);
    return *arg.typeinfo_ptr;
  }

  template <typename Tp>
  static constexpr bool IsValidCast() {
    return std::disjunction_v<std::is_reference<Tp>,
                              std::is_copy_constructible<Tp>>;
  }

 private:
  enum class Operator { kAccess, KGetTypeInfo, kClone, kDestroy, kXfer };
  union Arg {
    void* obj;
    const std::type_info* typeinfo_ptr;
    Any* any_ptr;
  };

  void (*manager_)(Operator, const Any*, Arg*);
  Storage storage_;

  template <typename Tp>
  friend void* AnyCaster(const Any* any);

  // Manage in-place contained object
  template <typename Tp>
  struct ManagerInternal {
    static void Manage(Operator which, const Any* any_ptr, Arg* arg);
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
    static void Manage(Operator which, const Any* any_ptr, Arg* arg);
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

// Exchange the states of two any objects
inline void Swap(Any& x, Any& y) noexcept { x.Swap(y); }

// Create an any holding a Tp constructed form args
template <typename Tp, typename... Args>
Any MakeAny(Args&&... args) {
  return Any(std::in_place_type<Tp>, std::forward<Args>(args)...);
}

// Create an any holding a Tp constructed from il and args
template <typename Tp, typename Up, typename... Args>
Any MakeAny(std::initializer_list<Up> il, Args&&... args) {
  return Any(std::in_place_type<Tp>, il, std::forward<Args>(args)...);
}

template <typename Tp>
using _AnyCast = std::remove_cv_t<std::remove_reference_t<Tp>>;

template <typename ValueType>
inline ValueType AnyCast(const Any& any) {
  static_assert(
      Any::IsValidCast<ValueType>(),
      "Template argument must be a reference or CopyConstructible type");
  auto p = AnyCast<_AnyCast<ValueType>>(&any);
  if (p) return static_cast<ValueType>(*p);
  ThrowBadAnyCast();
}

template <typename ValueType>
inline ValueType AnyCast(Any& any) {
  static_assert(
      Any::IsValidCast<ValueType>(),
      "Template argument must be a reference or CopyConstructible type");
  auto p = AnyCast<_AnyCast<ValueType>>(&any);
  if (p) return static_cast<ValueType>(*p);
  ThrowBadAnyCast();
}

template <typename ValueType>
inline ValueType AnyCast(Any&& any) {
  static_assert(
      Any::IsValidCast<ValueType>(),
      "Template argument must be a reference or CopyConstructible type");
  auto p = AnyCast<_AnyCast<ValueType>>(&any);
  if constexpr (!std::is_move_constructible_v<ValueType> ||
                std::is_lvalue_reference_v<ValueType>) {
    if (p) return static_cast<ValueType>(*p);
  } else {
    if (p) return static_cast<ValueType>(std::move(*p));
  }
  ThrowBadAnyCast();
}

template <typename Tp>
void* AnyCaster(const Any* any) {
  using Up = std::remove_cv_t<Tp>;
  if constexpr (!std::is_same_v<std::decay_t<Up>, Up>)
    return nullptr;
  else if constexpr (!std::is_copy_constructible_v<Up>)
    return nullptr;
  else if (any->manager_ == &Any::Manager<Up>::Manage ||
           any->Type() == typeid(Tp)) {
    Any::Arg arg;
    any->manager_(Any::Operator::kAccess, any, &arg);
    return arg.obj;
  }
  return nullptr;
}

template <typename ValueType>
inline const ValueType* AnyCast(const Any* any) noexcept {
  if constexpr (std::is_object_v<ValueType>) {
    if (any) {
      return static_cast<ValueType*>(AnyCaster<ValueType>(any));
    }
  }
  return nullptr;
}

template <typename ValueType>
inline const ValueType* AnyCast(Any* any) noexcept {
  if constexpr (std::is_object_v<ValueType>) {
    if (any) {
      return static_cast<ValueType*>(AnyCaster<ValueType>(any));
    }
  }
  return nullptr;
}

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

}  // namespace cpp_idioms