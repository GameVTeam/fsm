// Copyright 2020 fanghr
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// 	documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// github.com/chfanghr/fsm

#ifndef FSM__FSM_H_
#define FSM__FSM_H_

#include <string>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <map>
#include <condition_variable>

// include/fsm/any.h
#if __cplusplus >= 201703L
#include <any>
#else

#include <typeinfo>
#include <type_traits>
#include <stdexcept>

#if defined(PARTICLE)
#if !defined(__cpp_exceptions) && !defined(ANY_IMPL_NO_EXCEPTIONS) && !defined(ANY_IMPL_EXCEPTIONS)
#   define ANY_IMPL_NO_EXCEPTIONS
# endif
#else
// you can opt-out of exceptions by definining ANY_IMPL_NO_EXCEPTIONS,
// but you must ensure not to cast badly when passing an `any' object to any_cast<T>(any)
#endif

#if defined(PARTICLE)
#if !defined(__cpp_rtti) && !defined(ANY_IMPL_NO_RTTI) && !defined(ANY_IMPL_RTTI)
#   define ANY_IMPL_NO_RTTI
# endif
#else
// you can opt-out of RTTI by defining ANY_IMPL_NO_RTTI,
// in order to disable functions working with the typeid of a type
#endif

namespace std {
class bad_any_cast : public std::bad_cast {
 public:
  const char *what() const noexcept override {
	return "bad any cast";
  }
};

class any final {
 public:
  /// Constructs an object of type any with an empty state.
  any() :
	  vtable(nullptr) {
  }

  /// Constructs an object of type any with an equivalent state as other.
  any(const any &rhs) :
	  vtable(rhs.vtable) {
	if (!rhs.empty()) {
	  rhs.vtable->copy(rhs.storage, this->storage);
	}
  }

  /// Constructs an object of type any with a state equivalent to the original state of other.
  /// rhs is left in a valid but otherwise unspecified state.
  any(any &&rhs) noexcept :
	  vtable(rhs.vtable) {
	if (!rhs.empty()) {
	  rhs.vtable->move(rhs.storage, this->storage);
	  rhs.vtable = nullptr;
	}
  }

  /// Same effect as this->clear().
  ~any() {
	this->clear();
  }

  /// Constructs an object of type any that contains an object of type T direct-initialized with std::forward<ValueType>(value).
  ///
  /// T shall satisfy the CopyConstructible requirements, otherwise the program is ill-formed.
  /// This is because an `any` may be copy constructed into another `any` at any time, so a copy should always be allowed.
  template<typename ValueType, typename = typename std::enable_if<!std::is_same<typename std::decay<ValueType>::type,
																				any>::value>::type>
  any(ValueType &&value) {
	static_assert(std::is_copy_constructible<typename std::decay<ValueType>::type>::value,
				  "T shall satisfy the CopyConstructible requirements.");
	this->construct(std::forward<ValueType>(value));
  }

  /// Has the same effect as any(rhs).swap(*this). No effects if an exception is thrown.
  any &operator=(const any &rhs) {
	any(rhs).swap(*this);
	return *this;
  }

  /// Has the same effect as any(std::move(rhs)).swap(*this).
  ///
  /// The state of *this is equivalent to the original state of rhs and rhs is left in a valid
  /// but otherwise unspecified state.
  any &operator=(any &&rhs) noexcept {
	any(std::move(rhs)).swap(*this);
	return *this;
  }

  /// Has the same effect as any(std::forward<ValueType>(value)).swap(*this). No effect if a exception is thrown.
  ///
  /// T shall satisfy the CopyConstructible requirements, otherwise the program is ill-formed.
  /// This is because an `any` may be copy constructed into another `any` at any time, so a copy should always be allowed.
  template<typename ValueType, typename = typename std::enable_if<!std::is_same<typename std::decay<ValueType>::type,
																				any>::value>::type>
  any &operator=(ValueType &&value) {
	static_assert(std::is_copy_constructible<typename std::decay<ValueType>::type>::value,
				  "T shall satisfy the CopyConstructible requirements.");
	any(std::forward<ValueType>(value)).swap(*this);
	return *this;
  }

  /// If not empty, destroys the contained object.
  void clear() noexcept {
	if (!empty()) {
	  this->vtable->destroy(storage);
	  this->vtable = nullptr;
	}
  }

  /// Returns true if *this has no contained object, otherwise false.
  bool empty() const noexcept {
	return this->vtable == nullptr;
  }

#ifndef ANY_IMPL_NO_RTTI
  /// If *this has a contained object of type T, typeid(T); otherwise typeid(void).
  const std::type_info &type() const noexcept {
	return empty() ? typeid(void) : this->vtable->type();
  }
#endif

  /// Exchange the states of *this and rhs.
  void swap(any &rhs) noexcept {
	if (this->vtable != rhs.vtable) {
	  any tmp(std::move(rhs));

	  // move from *this to rhs.
	  rhs.vtable = this->vtable;
	  if (this->vtable != nullptr) {
		this->vtable->move(this->storage, rhs.storage);
		//this->vtable = nullptr; -- unneeded, see below
	  }

	  // move from tmp (previously rhs) to *this.
	  this->vtable = tmp.vtable;
	  if (tmp.vtable != nullptr) {
		tmp.vtable->move(tmp.storage, this->storage);
		tmp.vtable = nullptr;
	  }
	} else // same types
	{
	  if (this->vtable != nullptr)
		this->vtable->swap(this->storage, rhs.storage);
	}
  }

 private: // Storage and Virtual Method Table

  union storage_union {
	using stack_storage_t = typename std::aligned_storage<2 * sizeof(void *), std::alignment_of<void *>::value>::type;

	void *dynamic;
	stack_storage_t stack;      // 2 words for e.g. shared_ptr
  };

  /// Base VTable specification.
  struct vtable_type {
	// Note: The caller is responssible for doing .vtable = nullptr after destructful operations
	// such as destroy() and/or move().

#ifndef ANY_IMPL_NO_RTTI
	/// The type of the object this vtable is for.
	const std::type_info &(*type)() noexcept;
#endif

	/// Destroys the object in the union.
	/// The state of the union after this call is unspecified, caller must ensure not to use src anymore.
	void (*destroy)(storage_union &) noexcept;

	/// Copies the **inner** content of the src union into the yet unitialized dest union.
	/// As such, both inner objects will have the same state, but on separate memory locations.
	void (*copy)(const storage_union &src, storage_union &dest);

	/// Moves the storage from src to the yet unitialized dest union.
	/// The state of src after this call is unspecified, caller must ensure not to use src anymore.
	void (*move)(storage_union &src, storage_union &dest) noexcept;

	/// Exchanges the storage between lhs and rhs.
	void (*swap)(storage_union &lhs, storage_union &rhs) noexcept;
  };

  /// VTable for dynamically allocated storage.
  template<typename T>
  struct vtable_dynamic {
#ifndef ANY_IMPL_NO_RTTI
	static const std::type_info &type() noexcept {
	  return typeid(T);
	}
#endif

	static void destroy(storage_union &storage) noexcept {
	  //assert(reinterpret_cast<T*>(storage.dynamic));
	  delete reinterpret_cast<T *>(storage.dynamic);
	}

	static void copy(const storage_union &src, storage_union &dest) {
	  dest.dynamic = new T(*reinterpret_cast<const T *>(src.dynamic));
	}

	static void move(storage_union &src, storage_union &dest) noexcept {
	  dest.dynamic = src.dynamic;
	  src.dynamic = nullptr;
	}

	static void swap(storage_union &lhs, storage_union &rhs) noexcept {
	  // just exchage the storage pointers.
	  std::swap(lhs.dynamic, rhs.dynamic);
	}
  };

  /// VTable for stack allocated storage.
  template<typename T>
  struct vtable_stack {
#ifndef ANY_IMPL_NO_RTTI
	static const std::type_info &type() noexcept {
	  return typeid(T);
	}
#endif

	static void destroy(storage_union &storage) noexcept {
	  reinterpret_cast<T *>(&storage.stack)->~T();
	}

	static void copy(const storage_union &src, storage_union &dest) {
	  new(&dest.stack) T(reinterpret_cast<const T &>(src.stack));
	}

	static void move(storage_union &src, storage_union &dest) noexcept {
	  // one of the conditions for using vtable_stack is a nothrow move constructor,
	  // so this move constructor will never throw a exception.
	  new(&dest.stack) T(std::move(reinterpret_cast<T &>(src.stack)));
	  destroy(src);
	}

	static void swap(storage_union &lhs, storage_union &rhs) noexcept {
	  storage_union tmp_storage;
	  move(rhs, tmp_storage);
	  move(lhs, rhs);
	  move(tmp_storage, lhs);
	}
  };

  /// Whether the type T must be dynamically allocated or can be stored on the stack.
  template<typename T>
  struct requires_allocation :
	  std::integral_constant<bool,
							 !(std::is_nothrow_move_constructible<T>::value      // N4562 §6.3/3 [any.class]
								 && sizeof(T) <= sizeof(storage_union::stack)
								 && std::alignment_of<T>::value
									 <= std::alignment_of<storage_union::stack_storage_t>::value)> {
  };

  /// Returns the pointer to the vtable of the type T.
  template<typename T>
  static vtable_type *vtable_for_type() {
	using VTableType = typename std::conditional<requires_allocation<T>::value,
												 vtable_dynamic<T>,
												 vtable_stack<T>>::type;
	static vtable_type table = {
#ifndef ANY_IMPL_NO_RTTI
		VTableType::type,
#endif
		VTableType::destroy,
		VTableType::copy, VTableType::move,
		VTableType::swap,
	};
	return &table;
  }

 protected:
  template<typename T>
  friend const T *any_cast(const any *operand) noexcept;
  template<typename T>
  friend T *any_cast(any *operand) noexcept;

  /// Casts (with no type_info checks) the storage pointer as const T*.
  template<typename T>
  const T *cast() const noexcept {
	return requires_allocation<typename std::decay<T>::type>::value ?
		   reinterpret_cast<const T *>(storage.dynamic) :
		   reinterpret_cast<const T *>(&storage.stack);
  }

  /// Casts (with no type_info checks) the storage pointer as T*.
  template<typename T>
  T *cast() noexcept {
	return requires_allocation<typename std::decay<T>::type>::value ?
		   reinterpret_cast<T *>(storage.dynamic) :
		   reinterpret_cast<T *>(&storage.stack);
  }

 private:
  storage_union storage; // on offset(0) so no padding for align
  vtable_type *vtable;

  template<typename ValueType, typename T>
  typename std::enable_if<requires_allocation<T>::value>::type
  do_construct(ValueType &&value) {
	storage.dynamic = new T(std::forward<ValueType>(value));
  }

  template<typename ValueType, typename T>
  typename std::enable_if<!requires_allocation<T>::value>::type
  do_construct(ValueType &&value) {
	new(&storage.stack) T(std::forward<ValueType>(value));
  }

  /// Chooses between stack and dynamic allocation for the type decay_t<ValueType>,
  /// assigns the correct vtable, and constructs the object on our storage.
  template<typename ValueType>
  void construct(ValueType &&value) {
	using T = typename std::decay<ValueType>::type;

	this->vtable = vtable_for_type<T>();

	do_construct<ValueType, T>(std::forward<ValueType>(value));
  }
};

namespace detail {
template<typename ValueType>
inline ValueType any_cast_move_if_true(typename std::remove_reference<ValueType>::type *p, std::true_type) {
  return std::move(*p);
}

template<typename ValueType>
inline ValueType any_cast_move_if_true(typename std::remove_reference<ValueType>::type *p, std::false_type) {
  return *p;
}
}

/// Performs *any_cast<add_const_t<remove_reference_t<ValueType>>>(&operand), or throws bad_any_cast on failure.
template<typename ValueType>
inline ValueType any_cast(const any &operand) {
  auto p = any_cast<typename std::add_const<typename std::remove_reference<ValueType>::type>::type>(&operand);
#ifndef ANY_IMPL_NO_EXCEPTIONS
  if (p == nullptr) throw bad_any_cast();
#endif
  return *p;
}

/// Performs *any_cast<remove_reference_t<ValueType>>(&operand), or throws bad_any_cast on failure.
template<typename ValueType>
inline ValueType any_cast(any &operand) {
  auto p = any_cast<typename std::remove_reference<ValueType>::type>(&operand);
#ifndef ANY_IMPL_NO_EXCEPTIONS
  if (p == nullptr) throw bad_any_cast();
#endif
  return *p;
}

///
/// If ValueType is MoveConstructible and isn't a lvalue reference, performs
/// std::move(*any_cast<remove_reference_t<ValueType>>(&operand)), otherwise
/// *any_cast<remove_reference_t<ValueType>>(&operand). Throws bad_any_cast on failure.
///
template<typename ValueType>
inline ValueType any_cast(any &&operand) {
  using can_move = std::integral_constant<bool,
										  std::is_move_constructible<ValueType>::value
											  && !std::is_lvalue_reference<ValueType>::value>;

  auto p = any_cast<typename std::remove_reference<ValueType>::type>(&operand);
#ifndef ANY_IMPL_NO_EXCEPTIONS
  if (p == nullptr) throw bad_any_cast();
#endif
  return detail::any_cast_move_if_true<ValueType>(p, can_move());
}

/// If operand != nullptr && operand->type() == typeid(ValueType), a pointer to the object
/// contained by operand, otherwise nullptr.
template<typename ValueType>
inline const ValueType *any_cast(const any *operand) noexcept {
  using T = typename std::decay<ValueType>::type;

  if (operand && operand->vtable == any::vtable_for_type<T>())
	return operand->cast<ValueType>();
  else
	return nullptr;
}

/// If operand != nullptr && operand->type() == typeid(ValueType), a pointer to the object
/// contained by operand, otherwise nullptr.
template<typename ValueType>
inline ValueType *any_cast(any *operand) noexcept {
  using T = typename std::decay<ValueType>::type;

  if (operand && operand->vtable == any::vtable_for_type<T>())
	return operand->cast<ValueType>();
  else
	return nullptr;
}

inline void swap(any &lhs, any &rhs) noexcept {
  lhs.swap(rhs);
}
}

#endif

// include/fsm/optional.h

#if __cplusplus >= 201703L
#include <optional>
#else

# include <utility>
# include <type_traits>
# include <initializer_list>
# include <cassert>
# include <functional>
# include <string>
# include <stdexcept>

# define TR2_OPTIONAL_REQUIRES(...) typename enable_if<__VA_ARGS__::value, bool>::type = false

# if defined __GNUC__ // NOTE: GNUC is also defined for Clang
#   if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)
#     define TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
#   elif (__GNUC__ > 4)
#     define TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
#   endif
#

#   if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)
#     define TR2_OPTIONAL_GCC_4_7_AND_HIGHER___
#   elif (__GNUC__ > 4)
#     define TR2_OPTIONAL_GCC_4_7_AND_HIGHER___
#   endif
#

#   if (__GNUC__ == 4) && (__GNUC_MINOR__ == 8) && (__GNUC_PATCHLEVEL__ >= 1)
#     define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   elif (__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)
#     define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   elif (__GNUC__ > 4)
#     define TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   endif
# endif
#

# if defined __clang_major__
#   if (__clang_major__ == 3 && __clang_minor__ >= 5)
#     define TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#   elif (__clang_major__ > 3)
#     define TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#   endif
#   if defined TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_
#     define TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
#   elif (__clang_major__ == 3 && __clang_minor__ == 4 && __clang_patchlevel__ >= 2)
#     define TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
#   endif
# endif
#

# if defined _MSC_VER
#   if (_MSC_VER >= 1900)
#     define TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#   endif
# endif

# if defined __clang__
#   if (__clang_major__ > 2) || (__clang_major__ == 2) && (__clang_minor__ >= 9)
#     define OPTIONAL_HAS_THIS_RVALUE_REFS 1
#   else
#     define OPTIONAL_HAS_THIS_RVALUE_REFS 0
#   endif
# elif defined TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   define OPTIONAL_HAS_THIS_RVALUE_REFS 1
# elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
#   define OPTIONAL_HAS_THIS_RVALUE_REFS 1
# else
#   define OPTIONAL_HAS_THIS_RVALUE_REFS 0
# endif

# if defined TR2_OPTIONAL_GCC_4_8_1_AND_HIGHER___
#   define OPTIONAL_HAS_CONSTEXPR_INIT_LIST 1
#   define OPTIONAL_CONSTEXPR_INIT_LIST constexpr
# else
#   define OPTIONAL_HAS_CONSTEXPR_INIT_LIST 0
#   define OPTIONAL_CONSTEXPR_INIT_LIST
# endif

# if defined TR2_OPTIONAL_CLANG_3_5_AND_HIGHTER_ && (defined __cplusplus) && (__cplusplus != 201103L)
#   define OPTIONAL_HAS_MOVE_ACCESSORS 1
# else
#   define OPTIONAL_HAS_MOVE_ACCESSORS 0
# endif

# // In C++11 constexpr implies const, so we need to make non-const members also non-constexpr
# if (defined __cplusplus) && (__cplusplus == 201103L)
#   define OPTIONAL_MUTABLE_CONSTEXPR
# else
#   define OPTIONAL_MUTABLE_CONSTEXPR constexpr
# endif

namespace std {

// BEGIN workaround for missing is_trivially_destructible
# if defined TR2_OPTIONAL_GCC_4_8_AND_HIGHER___
// leave it: it is already there
# elif defined TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
// leave it: it is already there
# elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
// leave it: it is already there
# elif defined TR2_OPTIONAL_DISABLE_EMULATION_OF_TYPE_TRAITS
// leave it: the user doesn't want it
# else
template <typename T>
using is_trivially_destructible = std::has_trivial_destructor<T>;
# endif
// END workaround for missing is_trivially_destructible

# if (defined TR2_OPTIONAL_GCC_4_7_AND_HIGHER___)
// leave it; our metafunctions are already defined.
# elif defined TR2_OPTIONAL_CLANG_3_4_2_AND_HIGHER_
// leave it; our metafunctions are already defined.
# elif defined TR2_OPTIONAL_MSVC_2015_AND_HIGHER___
// leave it: it is already there
# elif defined TR2_OPTIONAL_DISABLE_EMULATION_OF_TYPE_TRAITS
// leave it: the user doesn't want it
# else


// workaround for missing traits in GCC and CLANG
template <class T>
struct is_nothrow_move_constructible
{
constexpr static bool value = std::is_nothrow_constructible<T, T&&>::value;
};


template <class T, class U>
struct is_assignable
{
template <class X, class Y>
constexpr static bool has_assign(...) { return false; }

template <class X, class Y, size_t S = sizeof((std::declval<X>() = std::declval<Y>(), true)) >
// the comma operator is necessary for the cases where operator= returns void
constexpr static bool has_assign(bool) { return true; }

constexpr static bool value = has_assign<T, U>(true);
};


template <class T>
struct is_nothrow_move_assignable
{
template <class X, bool has_any_move_assign>
struct has_nothrow_move_assign {
constexpr static bool value = false;
};

template <class X>
struct has_nothrow_move_assign<X, true> {
constexpr static bool value = noexcept( std::declval<X&>() = std::declval<X&&>() );
};

constexpr static bool value = has_nothrow_move_assign<T, is_assignable<T&, T&&>::value>::value;
};
// end workaround


# endif

// 20.5.4, optional for object types
template<class T>
class optional;

// 20.5.5, optional for lvalue reference types
template<class T>
class optional<T &>;

// workaround: std utility functions aren't constexpr yet
template<class T>
inline constexpr T &&constexpr_forward(typename std::remove_reference<T>::type &t) noexcept {
  return static_cast<T &&>(t);
}

template<class T>
inline constexpr T &&constexpr_forward(typename std::remove_reference<T>::type &&t) noexcept {
  static_assert(!std::is_lvalue_reference<T>::value, "!!");
  return static_cast<T &&>(t);
}

template<class T>
inline constexpr typename std::remove_reference<T>::type &&constexpr_move(T &&t) noexcept {
  return static_cast<typename std::remove_reference<T>::type &&>(t);
}

#if defined NDEBUG
# define TR2_OPTIONAL_ASSERTED_EXPRESSION(CHECK, EXPR) (EXPR)
#else
# define TR2_OPTIONAL_ASSERTED_EXPRESSION(CHECK, EXPR) ((CHECK) ? (EXPR) : ([]{assert(!#CHECK);}(), (EXPR)))
#endif

namespace detail_ {

// static_addressof: a constexpr version of addressof
template<typename T>
struct has_overloaded_addressof {
  template<class X>
  constexpr static bool has_overload(...) { return false; }

  template<class X, size_t S = sizeof(std::declval<X &>().operator&())>
  constexpr static bool has_overload(bool) { return true; }

  constexpr static bool value = has_overload<T>(true);
};

template<typename T, TR2_OPTIONAL_REQUIRES(!has_overloaded_addressof<T>)>
constexpr T *static_addressof(T &ref) {
  return &ref;
}

template<typename T, TR2_OPTIONAL_REQUIRES(has_overloaded_addressof<T>)>
T *static_addressof(T &ref) {
  return std::addressof(ref);
}

// the call to convert<A>(b) has return type A and converts b to type A iff b decltype(b) is implicitly convertible to A
template<class U>
constexpr U convert(U v) { return v; }

namespace swap_ns {
using std::swap;

template<class T>
void adl_swap(T &t, T &u) noexcept(noexcept(swap(t, u))) {
  swap(t, u);
}

} // namespace swap_ns

} // namespace detail


constexpr struct trivial_init_t {} trivial_init{};

// 20.5.6, In-place construction
constexpr struct in_place_t {} in_place{};

// 20.5.7, Disengaged state indicator
struct nullopt_t {
  struct init {};
  constexpr explicit nullopt_t(init) {}
};
constexpr nullopt_t nullopt{nullopt_t::init()};

// 20.5.8, class bad_optional_access
class bad_optional_access : public logic_error {
 public:
  explicit bad_optional_access(const string &what_arg) : logic_error{what_arg} {}
  explicit bad_optional_access(const char *what_arg) : logic_error{what_arg} {}
};

template<class T>
union storage_t {
  unsigned char dummy_;
  T value_;

  constexpr storage_t(trivial_init_t) noexcept : dummy_() {};

  template<class... Args>
  constexpr storage_t(Args &&... args) : value_(constexpr_forward<Args>(args)...) {}

  ~storage_t() {}
};

template<class T>
union constexpr_storage_t {
  unsigned char dummy_;
  T value_;

  constexpr constexpr_storage_t(trivial_init_t) noexcept : dummy_() {};

  template<class... Args>
  constexpr constexpr_storage_t(Args &&... args) : value_(constexpr_forward<Args>(args)...) {}

  ~constexpr_storage_t() = default;
};

template<class T>
struct optional_base {
  bool init_;
  storage_t<T> storage_;

  constexpr optional_base() noexcept : init_(false), storage_(trivial_init) {};

  explicit constexpr optional_base(const T &v) : init_(true), storage_(v) {}

  explicit constexpr optional_base(T &&v) : init_(true), storage_(constexpr_move(v)) {}

  template<class... Args>
  explicit optional_base(in_place_t, Args &&... args)
	  : init_(true), storage_(constexpr_forward<Args>(args)...) {}

  template<class U, class... Args, TR2_OPTIONAL_REQUIRES(is_constructible<T, std::initializer_list<U>>)>
  explicit optional_base(in_place_t, std::initializer_list<U> il, Args &&... args)
	  : init_(true), storage_(il, std::forward<Args>(args)...) {}

  ~optional_base() { if (init_) storage_.value_.T::~T(); }
};

template<class T>
struct constexpr_optional_base {
  bool init_;
  constexpr_storage_t<T> storage_;

  constexpr constexpr_optional_base() noexcept : init_(false), storage_(trivial_init) {};

  explicit constexpr constexpr_optional_base(const T &v) : init_(true), storage_(v) {}

  explicit constexpr constexpr_optional_base(T &&v) : init_(true), storage_(constexpr_move(v)) {}

  template<class... Args>
  explicit constexpr constexpr_optional_base(in_place_t, Args &&... args)
	  : init_(true), storage_(constexpr_forward<Args>(args)...) {}

  template<class U, class... Args, TR2_OPTIONAL_REQUIRES(is_constructible<T, std::initializer_list<U>>)>
  OPTIONAL_CONSTEXPR_INIT_LIST explicit constexpr_optional_base(in_place_t,
																std::initializer_list<U> il,
																Args &&... args)
	  : init_(true), storage_(il, std::forward<Args>(args)...) {}

  ~constexpr_optional_base() = default;
};

template<class T>
using OptionalBase = typename std::conditional<
	is_trivially_destructible<T>::value,                          // if possible
	constexpr_optional_base<typename std::remove_const<T>::type>, // use base with trivial destructor
	optional_base<typename std::remove_const<T>::type>
>::type;

template<class T>
class optional : private OptionalBase<T> {
  static_assert(!std::is_same<typename std::decay<T>::type, nullopt_t>::value, "bad T");
  static_assert(!std::is_same<typename std::decay<T>::type, in_place_t>::value, "bad T");

  constexpr bool initialized() const noexcept { return OptionalBase<T>::init_; }
  typename std::remove_const<T>::type *dataptr() { return std::addressof(OptionalBase<T>::storage_.value_); }
  constexpr const T *dataptr() const { return detail_::static_addressof(OptionalBase<T>::storage_.value_); }

# if OPTIONAL_HAS_THIS_RVALUE_REFS == 1
  constexpr const T &contained_val() const &{ return OptionalBase<T>::storage_.value_; }
#   if OPTIONAL_HAS_MOVE_ACCESSORS == 1
  OPTIONAL_MUTABLE_CONSTEXPR T&& contained_val() && { return std::move(OptionalBase<T>::storage_.value_); }
  OPTIONAL_MUTABLE_CONSTEXPR T& contained_val() & { return OptionalBase<T>::storage_.value_; }
#   else
  T &contained_val() &{ return OptionalBase<T>::storage_.value_; }
  T &&contained_val() &&{ return std::move(OptionalBase<T>::storage_.value_); }
#   endif
# else
  constexpr const T& contained_val() const { return OptionalBase<T>::storage_.value_; }
  T& contained_val() { return OptionalBase<T>::storage_.value_; }
# endif

  void clear() noexcept {
	if (initialized()) dataptr()->T::~T();
	OptionalBase<T>::init_ = false;
  }

  template<class... Args>
  void initialize(Args &&... args) noexcept(noexcept(T(std::forward<Args>(args)...))) {
	assert(!OptionalBase<T>::init_);
	::new(static_cast<void *>(dataptr())) T(std::forward<Args>(args)...);
	OptionalBase<T>::init_ = true;
  }

  template<class U, class... Args>
  void initialize(std::initializer_list<U> il, Args &&... args) noexcept(noexcept(T(il, std::forward<Args>(args)...))) {
	assert(!OptionalBase<T>::init_);
	::new(static_cast<void *>(dataptr())) T(il, std::forward<Args>(args)...);
	OptionalBase<T>::init_ = true;
  }

 public:
  typedef T value_type;

  // 20.5.5.1, constructors
  constexpr optional() noexcept : OptionalBase<T>() {};
  constexpr optional(nullopt_t) noexcept : OptionalBase<T>() {};

  optional(const optional &rhs)
	  : OptionalBase<T>() {
	if (rhs.initialized()) {
	  ::new(static_cast<void *>(dataptr())) T(*rhs);
	  OptionalBase<T>::init_ = true;
	}
  }

  optional(optional &&rhs) noexcept(is_nothrow_move_constructible<T>::value)
	  : OptionalBase<T>() {
	if (rhs.initialized()) {
	  ::new(static_cast<void *>(dataptr())) T(std::move(*rhs));
	  OptionalBase<T>::init_ = true;
	}
  }

  constexpr optional(const T &v) : OptionalBase<T>(v) {}

  constexpr optional(T &&v) : OptionalBase<T>(constexpr_move(v)) {}

  template<class... Args>
  explicit constexpr optional(in_place_t, Args &&... args)
	  : OptionalBase<T>(in_place_t{}, constexpr_forward<Args>(args)...) {}

  template<class U, class... Args, TR2_OPTIONAL_REQUIRES(is_constructible<T, std::initializer_list<U>>)>
  OPTIONAL_CONSTEXPR_INIT_LIST explicit optional(in_place_t, std::initializer_list<U> il, Args &&... args)
	  : OptionalBase<T>(in_place_t{}, il, constexpr_forward<Args>(args)...) {}

  // 20.5.4.2, Destructor
  ~optional() = default;

  // 20.5.4.3, assignment
  optional &operator=(nullopt_t) noexcept {
	clear();
	return *this;
  }

  optional &operator=(const optional &rhs) {
	if (initialized() == true && rhs.initialized() == false) clear();
	else if (initialized() == false && rhs.initialized() == true) initialize(*rhs);
	else if (initialized() == true && rhs.initialized() == true) contained_val() = *rhs;
	return *this;
  }

  optional &operator=(optional &&rhs)
  noexcept(is_nothrow_move_assignable<T>::value && is_nothrow_move_constructible<T>::value) {
	if (initialized() == true && rhs.initialized() == false) clear();
	else if (initialized() == false && rhs.initialized() == true) initialize(std::move(*rhs));
	else if (initialized() == true && rhs.initialized() == true) contained_val() = std::move(*rhs);
	return *this;
  }

  template<class U>
  auto operator=(U &&v)
  -> typename enable_if
	  <
		  is_same<typename decay<U>::type, T>::value,
		  optional &
	  >::type {
	if (initialized()) { contained_val() = std::forward<U>(v); }
	else { initialize(std::forward<U>(v)); }
	return *this;
  }

  template<class... Args>
  void emplace(Args &&... args) {
	clear();
	initialize(std::forward<Args>(args)...);
  }

  template<class U, class... Args>
  void emplace(initializer_list<U> il, Args &&... args) {
	clear();
	initialize<U, Args...>(il, std::forward<Args>(args)...);
  }

  // 20.5.4.4, Swap
  void swap(optional<T> &rhs) noexcept(is_nothrow_move_constructible<T>::value
	  && noexcept(detail_::swap_ns::adl_swap(declval<T &>(), declval<T &>()))) {
	if (initialized() == true && rhs.initialized() == false) {
	  rhs.initialize(std::move(**this));
	  clear();
	} else if (initialized() == false && rhs.initialized() == true) {
	  initialize(std::move(*rhs));
	  rhs.clear();
	} else if (initialized() == true && rhs.initialized() == true) { using std::swap; swap(**this, *rhs); }
  }

  // 20.5.4.5, Observers

  explicit constexpr operator bool() const noexcept { return initialized(); }
  constexpr bool has_value() const noexcept { return initialized(); }

  constexpr T const *operator->() const {
	return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), dataptr());
  }

# if OPTIONAL_HAS_MOVE_ACCESSORS == 1

  OPTIONAL_MUTABLE_CONSTEXPR T* operator ->() {
	assert (initialized());
	return dataptr();
  }

  constexpr T const& operator *() const& {
	return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), contained_val());
  }

  OPTIONAL_MUTABLE_CONSTEXPR T& operator *() & {
	assert (initialized());
	return contained_val();
  }

  OPTIONAL_MUTABLE_CONSTEXPR T&& operator *() && {
	assert (initialized());
	return constexpr_move(contained_val());
  }

  constexpr T const& value() const& {
	return initialized() ? contained_val() : (throw bad_optional_access("bad optional access"), contained_val());
  }

  OPTIONAL_MUTABLE_CONSTEXPR T& value() & {
	return initialized() ? contained_val() : (throw bad_optional_access("bad optional access"), contained_val());
  }

  OPTIONAL_MUTABLE_CONSTEXPR T&& value() && {
	if (!initialized()) throw bad_optional_access("bad optional access");
	return std::move(contained_val());
  }

# else

  T *operator->() {
	assert (initialized());
	return dataptr();
  }

  constexpr T const &operator*() const {
	return TR2_OPTIONAL_ASSERTED_EXPRESSION(initialized(), contained_val());
  }

  T &operator*() {
	assert (initialized());
	return contained_val();
  }

  constexpr T const &value() const {
	return initialized() ? contained_val() : (throw bad_optional_access("bad optional access"), contained_val());
  }

  T &value() {
	return initialized() ? contained_val() : (throw bad_optional_access("bad optional access"), contained_val());
  }

# endif

# if OPTIONAL_HAS_THIS_RVALUE_REFS == 1

  template<class V>
  constexpr T value_or(V &&v) const &{
	return *this ? **this : detail_::convert<T>(constexpr_forward<V>(v));
  }

#   if OPTIONAL_HAS_MOVE_ACCESSORS == 1

  template <class V>
  OPTIONAL_MUTABLE_CONSTEXPR T value_or(V&& v) &&
  {
	return *this ? constexpr_move(const_cast<optional<T>&>(*this).contained_val()) : detail_::convert<T>(constexpr_forward<V>(v));
  }

#   else

  template<class V>
  T value_or(V &&v) &&{
	return *this ? constexpr_move(const_cast<optional<T> &>(*this).contained_val()) : detail_::convert<T>(
		constexpr_forward<V>(v));
  }

#   endif

# else

  template <class V>
  constexpr T value_or(V&& v) const
  {
	return *this ? **this : detail_::convert<T>(constexpr_forward<V>(v));
  }

# endif

  // 20.6.3.6, modifiers
  void reset() noexcept { clear(); }
};

template<class T>
class optional<T &> {
  static_assert(!std::is_same<T, nullopt_t>::value, "bad T");
  static_assert(!std::is_same<T, in_place_t>::value, "bad T");
  T *ref;

 public:

  // 20.5.5.1, construction/destruction
  constexpr optional() noexcept : ref(nullptr) {}

  constexpr optional(nullopt_t) noexcept : ref(nullptr) {}

  constexpr optional(T &v) noexcept : ref(detail_::static_addressof(v)) {}

  optional(T &&) = delete;

  constexpr optional(const optional &rhs) noexcept : ref(rhs.ref) {}

  explicit constexpr optional(in_place_t, T &v) noexcept : ref(detail_::static_addressof(v)) {}

  explicit optional(in_place_t, T &&) = delete;

  ~optional() = default;

  // 20.5.5.2, mutation
  optional &operator=(nullopt_t) noexcept {
	ref = nullptr;
	return *this;
  }

  // optional& operator=(const optional& rhs) noexcept {
  // ref = rhs.ref;
  // return *this;
  // }

  // optional& operator=(optional&& rhs) noexcept {
  // ref = rhs.ref;
  // return *this;
  // }

  template<typename U>
  auto operator=(U &&rhs) noexcept
  -> typename enable_if
	  <
		  is_same<typename decay<U>::type, optional<T &>>::value,
		  optional &
	  >::type {
	ref = rhs.ref;
	return *this;
  }

  template<typename U>
  auto operator=(U &&rhs) noexcept
  -> typename enable_if
	  <
		  !is_same<typename decay<U>::type, optional<T &>>::value,
		  optional &
	  >::type
  = delete;

  void emplace(T &v) noexcept {
	ref = detail_::static_addressof(v);
  }

  void emplace(T &&) = delete;

  void swap(optional<T &> &rhs) noexcept {
	std::swap(ref, rhs.ref);
  }

  // 20.5.5.3, observers
  constexpr T *operator->() const {
	return TR2_OPTIONAL_ASSERTED_EXPRESSION(ref, ref);
  }

  constexpr T &operator*() const {
	return TR2_OPTIONAL_ASSERTED_EXPRESSION(ref, *ref);
  }

  constexpr T &value() const {
	return ref ? *ref : (throw bad_optional_access("bad optional access"), *ref);
  }

  explicit constexpr operator bool() const noexcept {
	return ref != nullptr;
  }

  constexpr bool has_value() const noexcept {
	return ref != nullptr;
  }

  template<class V>
  constexpr typename decay<T>::type value_or(V &&v) const {
	return *this ? **this : detail_::convert<typename decay<T>::type>(constexpr_forward<V>(v));
  }

  // x.x.x.x, modifiers
  void reset() noexcept { ref = nullptr; }
};

template<class T>
class optional<T &&> {
  static_assert(sizeof(T) == 0, "optional rvalue references disallowed");
};

// 20.5.8, Relational operators
template<class T>
constexpr bool operator==(const optional<T> &x, const optional<T> &y) {
  return bool(x) != bool(y) ? false : bool(x) == false ? true : *x == *y;
}

template<class T>
constexpr bool operator!=(const optional<T> &x, const optional<T> &y) {
  return !(x == y);
}

template<class T>
constexpr bool operator<(const optional<T> &x, const optional<T> &y) {
  return (!y) ? false : (!x) ? true : *x < *y;
}

template<class T>
constexpr bool operator>(const optional<T> &x, const optional<T> &y) {
  return (y < x);
}

template<class T>
constexpr bool operator<=(const optional<T> &x, const optional<T> &y) {
  return !(y < x);
}

template<class T>
constexpr bool operator>=(const optional<T> &x, const optional<T> &y) {
  return !(x < y);
}

// 20.5.9, Comparison with nullopt
template<class T>
constexpr bool operator==(const optional<T> &x, nullopt_t) noexcept {
  return (!x);
}

template<class T>
constexpr bool operator==(nullopt_t, const optional<T> &x) noexcept {
  return (!x);
}

template<class T>
constexpr bool operator!=(const optional<T> &x, nullopt_t) noexcept {
  return bool(x);
}

template<class T>
constexpr bool operator!=(nullopt_t, const optional<T> &x) noexcept {
  return bool(x);
}

template<class T>
constexpr bool operator<(const optional<T> &, nullopt_t) noexcept {
  return false;
}

template<class T>
constexpr bool operator<(nullopt_t, const optional<T> &x) noexcept {
  return bool(x);
}

template<class T>
constexpr bool operator<=(const optional<T> &x, nullopt_t) noexcept {
  return (!x);
}

template<class T>
constexpr bool operator<=(nullopt_t, const optional<T> &) noexcept {
  return true;
}

template<class T>
constexpr bool operator>(const optional<T> &x, nullopt_t) noexcept {
  return bool(x);
}

template<class T>
constexpr bool operator>(nullopt_t, const optional<T> &) noexcept {
  return false;
}

template<class T>
constexpr bool operator>=(const optional<T> &, nullopt_t) noexcept {
  return true;
}

template<class T>
constexpr bool operator>=(nullopt_t, const optional<T> &x) noexcept {
  return (!x);
}

// 20.5.10, Comparison with T
template<class T>
constexpr bool operator==(const optional<T> &x, const T &v) {
  return bool(x) ? *x == v : false;
}

template<class T>
constexpr bool operator==(const T &v, const optional<T> &x) {
  return bool(x) ? v == *x : false;
}

template<class T>
constexpr bool operator!=(const optional<T> &x, const T &v) {
  return bool(x) ? *x != v : true;
}

template<class T>
constexpr bool operator!=(const T &v, const optional<T> &x) {
  return bool(x) ? v != *x : true;
}

template<class T>
constexpr bool operator<(const optional<T> &x, const T &v) {
  return bool(x) ? *x < v : true;
}

template<class T>
constexpr bool operator>(const T &v, const optional<T> &x) {
  return bool(x) ? v > *x : true;
}

template<class T>
constexpr bool operator>(const optional<T> &x, const T &v) {
  return bool(x) ? *x > v : false;
}

template<class T>
constexpr bool operator<(const T &v, const optional<T> &x) {
  return bool(x) ? v < *x : false;
}

template<class T>
constexpr bool operator>=(const optional<T> &x, const T &v) {
  return bool(x) ? *x >= v : false;
}

template<class T>
constexpr bool operator<=(const T &v, const optional<T> &x) {
  return bool(x) ? v <= *x : false;
}

template<class T>
constexpr bool operator<=(const optional<T> &x, const T &v) {
  return bool(x) ? *x <= v : true;
}

template<class T>
constexpr bool operator>=(const T &v, const optional<T> &x) {
  return bool(x) ? v >= *x : true;
}

// Comparison of optional<T&> with T
template<class T>
constexpr bool operator==(const optional<T &> &x, const T &v) {
  return bool(x) ? *x == v : false;
}

template<class T>
constexpr bool operator==(const T &v, const optional<T &> &x) {
  return bool(x) ? v == *x : false;
}

template<class T>
constexpr bool operator!=(const optional<T &> &x, const T &v) {
  return bool(x) ? *x != v : true;
}

template<class T>
constexpr bool operator!=(const T &v, const optional<T &> &x) {
  return bool(x) ? v != *x : true;
}

template<class T>
constexpr bool operator<(const optional<T &> &x, const T &v) {
  return bool(x) ? *x < v : true;
}

template<class T>
constexpr bool operator>(const T &v, const optional<T &> &x) {
  return bool(x) ? v > *x : true;
}

template<class T>
constexpr bool operator>(const optional<T &> &x, const T &v) {
  return bool(x) ? *x > v : false;
}

template<class T>
constexpr bool operator<(const T &v, const optional<T &> &x) {
  return bool(x) ? v < *x : false;
}

template<class T>
constexpr bool operator>=(const optional<T &> &x, const T &v) {
  return bool(x) ? *x >= v : false;
}

template<class T>
constexpr bool operator<=(const T &v, const optional<T &> &x) {
  return bool(x) ? v <= *x : false;
}

template<class T>
constexpr bool operator<=(const optional<T &> &x, const T &v) {
  return bool(x) ? *x <= v : true;
}

template<class T>
constexpr bool operator>=(const T &v, const optional<T &> &x) {
  return bool(x) ? v >= *x : true;
}

// Comparison of optional<T const&> with T
template<class T>
constexpr bool operator==(const optional<const T &> &x, const T &v) {
  return bool(x) ? *x == v : false;
}

template<class T>
constexpr bool operator==(const T &v, const optional<const T &> &x) {
  return bool(x) ? v == *x : false;
}

template<class T>
constexpr bool operator!=(const optional<const T &> &x, const T &v) {
  return bool(x) ? *x != v : true;
}

template<class T>
constexpr bool operator!=(const T &v, const optional<const T &> &x) {
  return bool(x) ? v != *x : true;
}

template<class T>
constexpr bool operator<(const optional<const T &> &x, const T &v) {
  return bool(x) ? *x < v : true;
}

template<class T>
constexpr bool operator>(const T &v, const optional<const T &> &x) {
  return bool(x) ? v > *x : true;
}

template<class T>
constexpr bool operator>(const optional<const T &> &x, const T &v) {
  return bool(x) ? *x > v : false;
}

template<class T>
constexpr bool operator<(const T &v, const optional<const T &> &x) {
  return bool(x) ? v < *x : false;
}

template<class T>
constexpr bool operator>=(const optional<const T &> &x, const T &v) {
  return bool(x) ? *x >= v : false;
}

template<class T>
constexpr bool operator<=(const T &v, const optional<const T &> &x) {
  return bool(x) ? v <= *x : false;
}

template<class T>
constexpr bool operator<=(const optional<const T &> &x, const T &v) {
  return bool(x) ? *x <= v : true;
}

template<class T>
constexpr bool operator>=(const T &v, const optional<const T &> &x) {
  return bool(x) ? v >= *x : true;
}

// 20.5.12, Specialized algorithms
template<class T>
void swap(optional<T> &x, optional<T> &y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

template<class T>
constexpr optional<typename decay<T>::type> make_optional(T &&v) {
  return optional<typename decay<T>::type>(constexpr_forward<T>(v));
}

template<class X>
constexpr optional<X &> make_optional(reference_wrapper<X> v) {
  return optional<X &>(v.get());
}
} // namespace std

namespace std {
template<typename T>
struct hash<std::optional<T>> {
  typedef typename hash<T>::result_type result_type;
  typedef std::optional<T> argument_type;

  constexpr result_type operator()(argument_type const &arg) const {
	return arg ? std::hash<T>{}(*arg) : result_type{};
  }
};

template<typename T>
struct hash<std::optional<T &>> {
  typedef typename hash<T>::result_type result_type;
  typedef std::optional<T &> argument_type;

  constexpr result_type operator()(argument_type const &arg) const {
	return arg ? std::hash<T>{}(*arg) : result_type{};
  }
};
}

# undef TR2_OPTIONAL_REQUIRES
# undef TR2_OPTIONAL_ASSERTED_EXPRESSION
#endif

// include/fsm/errors.h
namespace fsm {
class Error {
 public:
  [[nodiscard]] virtual std::string What() const noexcept(false) = 0;

  class Hasher {
   public:
	size_t operator()(const Error &error) const noexcept(false) {
	  return std::hash<std::string>()(error.What());
	}
  };

  // TODO A better way to compare two errors.
  bool operator==(const Error &other) const noexcept(false) {
	Hasher hasher{};
	return hasher(*this) == hasher(other);
  }

  virtual ~Error() = default;
};

// InTransitionError is returned by FSM.Event() when an asynchronous transition
// is already in progress.
class InTransitionError : public Error {
 public:
  explicit InTransitionError(std::string event) : event_(std::move(event)) {}

  std::string event_;

  [[nodiscard]] std::string What() const noexcept(false) override {
	return "event " + event_ + " inappropriate because previous transition did not complete";
  }
};

// InvalidEventError is returned by FSM::Event() when the event cannot be called
// in the current state.
class InvalidEventError : public Error {
 public:
  std::string event_;
  std::string state_;

  InvalidEventError(std::string event, std::string state) : event_(std::move(event)), state_(std::move(state)) {}

  [[nodiscard]] std::string What() const noexcept(false) override {
	return "event " + event_ + " inappropriate in current state " + state_;
  }
};

// UnknownEventError is returned by FSM::Event() when the event is not defined.
class UnknownEventError : public Error {
 public:
  std::string event_;

  explicit UnknownEventError(std::string event) : event_(std::move(event)) {}

  [[nodiscard]] std::string What() const noexcept(false) override {
	return "event " + event_ + " does not exist";
  }
};

// NotInTransitionError is returned by FSM::Transition() when an asynchronous
// transition is not in progress.
class NotInTransitionError : public Error {
 public:
  [[nodiscard]] std::string What() const noexcept(false) override {
	return "transition inappropriate because no state change in progress";
  }
};

// NoTransitionError is returned by FSM::Event() when no transition have happened,
// for example if the source and destination states are the same.
class NoTransitionError : public Error {
 public:
  std::shared_ptr<Error> error_;

  explicit NoTransitionError(std::shared_ptr<Error> error) : error_(std::move(error)) {}

  [[nodiscard]] std::string What() const noexcept(false) override {
	if (error_)
	  return "no transition with error: " + error_->What();
	return "no transition";
  }
};

// CanceledError is returned by FSM::Event() when a callback have canceled a
// transition.
class CanceledError : public Error {
 public:
  std::shared_ptr<Error> error_;

  explicit CanceledError(std::shared_ptr<Error> error) : error_(std::move(error)) {}

  [[nodiscard]] std::string What() const noexcept(false) override {
	if (error_)
	  return "transition canceled with error: " + error_->What();
	return "transition canceled";
  }
};

// AsyncError is returned by FSM::Event() when a callback have initiated an
// asynchronous state transition.
class AsyncError : public Error {
 public:
  std::shared_ptr<Error> error_;

  explicit AsyncError(std::shared_ptr<Error> error) : error_(std::move(error)) {}

  [[nodiscard]] std::string What() const noexcept(false) override {
	if (error_)
	  return "async started with error: " + error_->What();
	return "async started";
  }
};

// InternalError is returned by FSM::Event() and should never occur. It is a
// probably because of a bug
class InternalError : public Error {
  [[nodiscard]] std::string What() const noexcept(false) override {
	return "internal error on state transition";
  }
};

class UnknownVisualizeTypeError : public Error {
 public:
  [[nodiscard]] std::string What() const noexcept(false) override {
	return "unknown visualize type";
  }
};
}

// include/fsm/events.h
namespace fsm {
class FSM;

// Event is the info that get passed as a reference in the collection.
class Event {
 private:
  friend class FSM;
 public:
  // machine_ is a reference to the current FSM.
  std::reference_wrapper<FSM> machine_;

  // event_ is the name of event.
  std::string event_{};

  // src_ is the state before transition.
  std::string src_{};

  // dst_ is the state after transition.
  std::string dst_{};

  // error_ is an optional error that can be returned from callback.
  std::optional<std::shared_ptr<Error>> error_{};

  // args_ is optional list of arguments passed to the callback.
  std::vector<std::any> args_{};

  explicit Event(FSM &machine) : machine_(machine) {}

 private:

  // canceled_ is an internal flag set if the transition is canceled.
  bool canceled_{};

  // async_ is an internal flag set if the transition should be asynchronous.
  bool async_{};

 public:
  // Cancel can be called in before_<EVENT> or leave_<State> to cancel the
  // current transition before it happens. It takes an optional error, which will
  // overwrite error_ if set before.
  void Cancel(const std::vector<std::shared_ptr<Error>> &errors = {}) noexcept;

  // Async can be called in leave_<STATE> to do an asynchronous state transition.
  //
  // The current state transition will be on hold in the old state until a final
  // call to Transition is made. This will comlete the transition and possibly
  // call the other callbacks.
  void Async() noexcept;
};
}

// include/fsm/locks.h
namespace fsm {
class RWLock {
 public:
  RWLock() : status_(0), waiting_readers_(0), waiting_writers_(0) {}
//  RWLock(const RWLock &) = delete;
//  RWLock(RWLock &&) = delete;
//  RWLock &operator=(const RWLock &) = delete;
//  RWLock &operator=(RWLock &&) = delete;

  void RLock() {
	std::unique_lock<std::mutex> lck(mtx_);
	waiting_readers_ += 1;
	read_cv_.wait(lck, [&]() { return waiting_writers_ == 0 && status_ >= 0; });
	waiting_readers_ -= 1;
	status_ += 1;
  }

  void Lock() {
	std::unique_lock<std::mutex> lck(mtx_);
	waiting_writers_ += 1;
	write_cv_.wait(lck, [&]() { return status_ == 0; });
	waiting_writers_ -= 1;
	status_ = -1;
  }

  void Unlock() {
	std::unique_lock<std::mutex> lck(mtx_);
	if (status_ == -1) {
	  status_ = 0;
	} else {
	  status_ -= 1;
	}
	if (waiting_writers_ > 0) {
	  if (status_ == 0) {
		write_cv_.notify_one();
	  }
	} else {
	  read_cv_.notify_all();
	}
  }

 private:
  // -1    : one writer
  // 0     : no reader and no writer
  // n > 0 : n reader
  int32_t status_;
  int32_t waiting_readers_;
  int32_t waiting_writers_;
  std::mutex mtx_;
  std::condition_variable read_cv_;
  std::condition_variable write_cv_;
};

class RLockGuard {
 private:
  RWLock &lock_;

 public:
  explicit RLockGuard(RWLock &lock) : lock_(lock) {
	lock_.RLock();
  }
  ~RLockGuard() {
	lock_.Unlock();
  }
};

class WLockGuard {
 private:
  RWLock &lock_;

 public:
  explicit WLockGuard(RWLock &lock) : lock_(lock) {
	lock_.Lock();
  }
  ~WLockGuard() {
	lock_.Unlock();
  }
};

class LockGuard {
 private:
  std::mutex &mu_;

 public:
  explicit LockGuard(std::mutex &mu) : mu_(mu) { mu_.lock(); }

  ~LockGuard() { mu_.unlock(); }
};
}

// include/fsm/utils.h
namespace fsm {
class FSM;
class Error;

// VisualizeType is the type of the visualization
enum class VisualizeType : int {
  kGraphviz,
  kMermaid,
};

using VisualizeResult=std::pair<std::string, std::optional<std::shared_ptr<Error>>>;

// Visualize outputs a visualization of a FSM in desired format.
// If the type is not given it defaults to Graphviz.
VisualizeResult Visualize(FSM &, VisualizeType) noexcept(false);
}

// include/fsm/fsm.h
namespace fsm {
class FSM;

namespace impl {

// Transitioner is a pure virtual class for the FSM's transition function.
class Transitioner {
 public:
  virtual std::optional<std::shared_ptr<Error>> Transition(FSM &) noexcept(false) = 0;
};

// TransitionerClass is the default implementation of the transitioner
class TransitionerClass : public Transitioner {
 public:
  // Transition completes an asynchronous state change.
  //
  // The callback for leave_<STATE> must previously have called Async on its
  // event to have initiated an asynchronous state transition.
  std::optional<std::shared_ptr<Error> > Transition(FSM &) noexcept(false) override;
};

enum class CallbackType : int {
  kNone,
  kBeforeEvent,
  kLeaveState,
  kEnterState,
  kAfterEvent
};

// CKey is used for keeping the callbacks mapped to a target.
class CKey {
 public:
  // target_ is either the name of a state or an event depending on which
  // callback type the key refers to. It can also be empty for a non-target.
  std::string target_;

  // callback_type_ is the situation when the callback will be run.
  CallbackType callback_type_;

  bool operator==(const CKey &other) const noexcept;

  // Hasher implements hash function to use std::unordered_map
  class Hasher {
   public :
	size_t operator()(const CKey &key) const noexcept(false);
  };
};

// EKey is used for storing the transition map.
class EKey {
 public:
  // event_ is the name of the event that the keys refers to.
  std::string event_;
  // src_ is the source from where the event can transition.
  std::string src_;

  bool operator==(const EKey &other) const noexcept;

  // Hasher implements hash function to use std::unordered_map
  class Hasher {
   public:
	size_t operator()(const EKey &key) const noexcept(false);
  };
};
}

// Callback is a function type that callbacks should use. Event is the current
// event info as the callback happens.
using Callback=std::function<void(Event &)>;

// Callbacks is a shorthand for defining the callbacks in the constructor of FSM.
using Callbacks=std::unordered_map<std::string, Callback>;

// EventDesc represents an event when initializing the FSM.
//
// The event can have one or more source states that is valid for performing
// the transition. If the FSM is in one of the source states it will end up in
// the specified destination state, calling all defined callbacks as it goes.
class EventDesc {
 public:
  // name_ is the event name used when calling for a transition.
  std::string name_;

  // src_ is a vector of source states that the FSM must be in to perform a
  // state transition.
  std::vector<std::string> src_;

  // dst_ is the destination state that the FSM will be in if the transition succeeds.
  std::string dst_;
};

// Events is a shorthand for defining the transition map in the constructor of FSM.
using Events=std::vector<EventDesc>;

// FSM is the state machine that holds the current state.
class FSM {
 private:
  friend class impl::TransitionerClass;
  friend class MermaidVisualizer;
  friend class GraphvizVisualizer;
 private:
  // current_ is the state that the FSM is currently in.
  std::string current_;

  // transitions_ maps events and source states to destination states.
  std::unordered_map<impl::EKey, std::string, impl::EKey::Hasher> transitions_{};

  // callbacks_ maps events and targets to callback functions.
  std::unordered_map<impl::CKey, Callback, impl::CKey::Hasher> callbacks_{};

  // transition_ is the internal transition functions used either directly
  // or when transition is called in an asynchronous state transition.
  std::function<void()> transition_{};

  // transition_obj_ calls the FSM::Transition() function.
  std::shared_ptr<impl::Transitioner> transition_obj_;

  // event_mu_ guards access to Event() and Transition().
  std::mutex event_mu_{};

  // state_mu_ guards access to the current state.
  RWLock state_mu_{};

 public:
  // Construct a FSM from events and callbacks.
  //
  // The events and transitions are specified as a vector of Event classes
  // specified as Events. Each Event is mapped to one or more internal
  // transitions from Event::src_ to Event::dst_.
  //
  // Callbacks are added as a map specified as Callbacks where the key is parsed
  // as the callback event as follows, and called in the same order:
  //
  // 1. before_<EVENT> - called before event named <EVENT>
  //
  // 2. before_event - called before all events
  //
  // 3. leave_<OLD_STATE> - called before leaving <OLD_STATE>
  //
  // 4. leave_state - called before leaving all states
  //
  // 5. enter_<NEW_STATE> - called after entering <NEW_STATE>
  //
  // 6. enter_state - called after entering all states
  //
  // 7. after_<EVENT> - called after event named <EVENT>
  //
  // 8. after_event - called after all events
  //
  // There are also two short form versions for the most commonly used callbacks.
  // They are simply the name of the event or state:
  //
  // 1. <NEW_STATE> - called after entering <NEW_STATE>
  //
  // 2. <EVENT> - called after event named <EVENT>
  //
  // If both a shorthand version and a full version is specified it is undefined
  // which version of the callback will end up in the internal map. This is due
  // to the psuedo random nature of Go maps. No checking for multiple keys is
  // currently performed.
  FSM(std::string initial, const Events &events, const Callbacks &callbacks = {}) noexcept(false);

  // Current returns the current state of FSM.
  std::string Current() noexcept(false);

  // Is returns true if state is the current state.
  bool Is(const std::string &state) noexcept(false);

  // SetState allows users to move to the given state from current state.
  // The call does not trigger any callbacks, if defined.
  void SetState(std::string state) noexcept(false);

  // Can returns true if the event can occur in the current state.
  bool Can(const std::string &event) noexcept(false);

  // Cannot returns true if event can not occur in the current state.
  // It's a convenience method to help code read nicely.
  bool Cannot(const std::string &event) noexcept(false);

  // AvailableTransitions returns a list of transitions available in the
  // current state.
  std::vector<std::string> AvailableTransitions() noexcept(false);

  // Transition wraps impl::Transitioner::Transition.
  std::optional<std::shared_ptr<Error>> Transition() noexcept(false);

  // FireEvent initiates a state transition with the named event.
  //
  // The call takes a variable number of arguments that will be passed to the
  // callback, if defined.
  //
  // It will return nil if the state change is ok or one of these errors:
  //
  // - event X inappropriate because previous transition did not complete
  //
  // - event X inappropriate in current state Y
  //
  // - event X does not exist
  //
  // - internal error on state transition
  //
  // The last error should never occur in this situation and is a sign of an
  // internal bug.
  std::optional<std::shared_ptr<Error>> FireEvent(const std::string &event,
												  std::vector<std::any> args = {}) noexcept(false);

  // Visualize outputs a visualization of this FSM in the desired format.
  // Note that this facility is not thread safety.
  VisualizeResult Visualize(VisualizeType visualize_type = VisualizeType::kGraphviz) noexcept(false);

  FSM(const FSM &fsm);
 private:
  // DoTransition wraps impl::Transitioner::Transition.
  std::optional<std::shared_ptr<Error>> DoTransition() noexcept(false);

  // AfterEventCallbacks calls the after_ callbacks, first the named then the
  // general version.
  void AfterEventCallbacks(Event &event) noexcept(false);

  // EnterStateCallbacks calls the enter_ callbacks, first the named then the
  // general version.
  void EnterStateCallbacks(Event &event) noexcept(false);

  // LeaveStateCallbacks calls the leave_ callbacks, first the named then the
  // general version.
  std::optional<std::shared_ptr<Error> > LeaveStateCallbacks(Event &event) noexcept(false);

  // BeforeEventCallbacks calls the before_ callbacks, first the named then the
  // general version.
  std::optional<std::shared_ptr<Error> > BeforeEventCallbacks(Event &event) noexcept(false);
};
}

// src/events.cc
namespace fsm {
void Event::Cancel(const std::vector<std::shared_ptr<Error>> &errors) noexcept {
  if (canceled_)
	return;
  canceled_ = true;
  if (!errors.empty())
	error_ = errors[0];
}

void Event::Async() noexcept {
  async_ = true;
}
}

// src/fsm.cc
namespace fsm {
FSM::FSM(std::string initial,
		 const std::vector<fsm::EventDesc> &events,
		 const std::unordered_map<std::string, std::function<void(Event &)>> &callbacks) noexcept(false)
	: current_(std::move(initial)), transition_obj_(std::make_shared<impl::TransitionerClass>()) {
  // Build transition map and store sets of all events and states.
  std::unordered_map<std::string, bool> all_events{}, all_states{};
  for (const auto &event:events) {
	for (const auto &src:event.src_) {
	  transitions_[impl::EKey{event.name_, src}] = event.dst_;
	  all_states[src] = true;
	  all_states[event.dst_] = true;
	}
	all_events[event.name_] = true;
  }

  // Map all callbacks to events/states.
  for (const auto &kv:callbacks) {
	// const auto&[name, fn]=kv;
	// TODO: I don't know why `name` cannot be captured by lambda using C++17 structured binding shown above.
	const std::string &name = kv.first;
	const Callback &fn = kv.second;

	std::string target{};
	impl::CallbackType callback_type{};

#if __cplusplus >= 201703L
	auto helper = [&](std::string_view prefix) -> bool {
#else
	auto helper = [&](const std::string &prefix) -> bool {
#endif
	  if (name.rfind(prefix) == 0) {
		target = name.substr(prefix.size());
		return true;
	  }
	  return false;
	};

	if (helper("before_")) {
	  if (target == "event") {
		target = "";
		callback_type = impl::CallbackType::kBeforeEvent;
	  } else if (all_events.find(target) != all_events.end())
		callback_type = impl::CallbackType::kBeforeEvent;
	} else if (helper("leave_")) {
	  if (target == "state") {
		target = "";
		callback_type = impl::CallbackType::kLeaveState;
	  } else if (all_states.find(target) != all_states.end())
		callback_type = impl::CallbackType::kLeaveState;
	} else if (helper("enter_")) {
	  if (target == "state") {
		target = "";
		callback_type = impl::CallbackType::kEnterState;
	  } else if (all_states.find(target) != all_states.end())
		callback_type = impl::CallbackType::kEnterState;
	} else if (helper("after_")) {
	  if (target == "event") {
		target = "";
		callback_type = impl::CallbackType::kAfterEvent;
	  } else if (all_events.find(target) != all_events.end())
		callback_type = impl::CallbackType::kAfterEvent;
	} else {
	  target = name;
	  if (all_states.find(target) != all_states.end())
		callback_type = impl::CallbackType::kEnterState;
	  else if (all_events.find(target) != all_events.end())
		callback_type = impl::CallbackType::kAfterEvent;
	}

	if (callback_type != impl::CallbackType::kNone)
	  callbacks_[impl::CKey{target, callback_type}] = fn;
  }
}

std::optional<std::shared_ptr<Error>> FSM::FireEvent(const std::string &event,
													 std::vector<std::any> args) noexcept(false) {
  LockGuard event_mu_guard(event_mu_);
  RLockGuard state_mu_guard(state_mu_);

  if (transition_)
	return {std::make_shared<InTransitionError>(event)};

  auto iter = transitions_.find(impl::EKey{event, current_});

  if (iter == transitions_.end()) {
	for (const auto &ekv:transitions_)
	  if (ekv.first.event_ == event)
		return {std::make_shared<InvalidEventError>(event, current_)};
	return {std::make_shared<UnknownEventError>(event)};
  }

  auto &dst = iter->second;

  auto event_obj = std::make_shared<Event>(*this);
  event_obj->event_ = event;
  event_obj->args_ = std::move(args);
  event_obj->src_ = current_;
  event_obj->dst_ = dst;

  auto err = BeforeEventCallbacks(*event_obj);

  if (err)
	return err;

  if (current_ == dst) {
	AfterEventCallbacks(*event_obj);
	//return {};
	return {std::make_shared<NoTransitionError>(event_obj->error_ ? event_obj->error_.value() : nullptr)};
  }

  // Setup the transition, call it later.
  transition_ = [=]() {
	state_mu_.Lock();
	current_ = dst;
	state_mu_.Unlock();

	EnterStateCallbacks(*event_obj);
	AfterEventCallbacks(*event_obj);
  };

  err = LeaveStateCallbacks(*event_obj);

  if (err) {
	if (std::dynamic_pointer_cast<CanceledError>(err.value()) != nullptr)
	  transition_ = nullptr;
	return err;
  }

  // Perform the rest of the transition, if not asynchronous.
  state_mu_.Unlock();
  err = DoTransition();
  state_mu_.RLock();

  if (err)
	return {std::make_shared<InternalError>()};

  return event_obj->error_;
}

std::string FSM::Current() noexcept(false) {
  RLockGuard guard(state_mu_);
  return current_;
}

bool FSM::Is(const std::string &state) noexcept(false) {
  RLockGuard guard(state_mu_);
  return state == current_;
}

void FSM::SetState(std::string state) noexcept(false) {
  WLockGuard guard(state_mu_);
  current_ = std::move(state);
}

bool FSM::Can(const std::string &event) noexcept(false) {
  RLockGuard guard(state_mu_);
  return transitions_.find(impl::EKey{event, current_}) != transitions_.end() && !transition_;
}

bool FSM::Cannot(const std::string &event) noexcept(false) {
  return !Can(event);
}

std::vector<std::string> FSM::AvailableTransitions() noexcept(false) {
  RLockGuard guard(state_mu_);
  std::vector<std::string> res{};
  for (const auto &kv:transitions_)
	if (kv.first.src_ == current_)
	  res.push_back(kv.first.event_);
  return res;
}

std::optional<std::shared_ptr<Error>> FSM::Transition() noexcept(false) {
  LockGuard guard(event_mu_);
  return DoTransition();
}

std::optional<std::shared_ptr<Error>> FSM::DoTransition() noexcept(false) {
  return transition_obj_->Transition(*this);
}

void FSM::AfterEventCallbacks(Event &event) noexcept(false) {
  auto iter = callbacks_.end();
  if ((iter = callbacks_.find(impl::CKey{
	  event.event_,
	  impl::CallbackType::kAfterEvent})) != callbacks_.end()) {
	iter->second(event);
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kAfterEvent
  })) != callbacks_.end()) {
	iter->second(event);
  }
}

void FSM::EnterStateCallbacks(Event &event) noexcept(false) {
  auto iter = callbacks_.end();
  if ((iter = callbacks_.find(impl::CKey{
	  current_,
	  impl::CallbackType::kEnterState
  })) != callbacks_.end()) {
	iter->second(event);
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kEnterState
  })) != callbacks_.end()) {
	iter->second(event);
  }
}

std::optional<std::shared_ptr<Error> > FSM::LeaveStateCallbacks(Event &event) noexcept(false) {
  auto iter = callbacks_.end();
  if ((iter = callbacks_.find(impl::CKey{
	  current_,
	  impl::CallbackType::kLeaveState
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr)};
	else if (event.async_)
	  return {std::make_shared<AsyncError>(event.error_ ? event.error_.value() : nullptr)};
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kLeaveState
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr)};
	else if (event.async_)
	  return {std::make_shared<AsyncError>(event.error_ ? event.error_.value() : nullptr)};
  }
  return {};
}

std::optional<std::shared_ptr<Error> > FSM::BeforeEventCallbacks(Event &event) noexcept(false) {
  auto iter = callbacks_.end();
  if ((iter = callbacks_.find(impl::CKey{
	  event.event_,
	  impl::CallbackType::kBeforeEvent
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr)};
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kBeforeEvent
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr)};
  }
  return {};
}

VisualizeResult FSM::Visualize(VisualizeType visualize_type) noexcept(false) {
  return ::fsm::Visualize(*this, visualize_type);
}

namespace impl {
std::optional<std::shared_ptr<Error> > TransitionerClass::Transition(FSM &machine) noexcept(false) {
  if (!machine.transition_) {
	return {std::make_shared<NotInTransitionError>()};
  }
  machine.transition_();
  WLockGuard guard(machine.state_mu_);
  machine.transition_ = nullptr;
  return {};
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
size_t CKey::Hasher::operator()(const CKey &key) const noexcept(false) {
  return (std::hash<std::string>()(key.target_) ^ (std::hash<CallbackType>()(key.callback_type_) << 1)) >> 1;
}
#pragma clang diagnostic pop

bool EKey::operator==(const EKey &other) const noexcept {
  return event_ == other.event_ && src_ == other.src_;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
size_t EKey::Hasher::operator()(const EKey &key) const noexcept(false) {
  return (std::hash<std::string>()(key.event_) ^ (std::hash<std::string>()(key.src_) << 1)) >> 1;
}
#pragma clang diagnostic pop

bool CKey::operator==(const CKey &other) const noexcept {
  return target_ == other.target_ && callback_type_ == other.callback_type_;
}
}
}

// src/utils.cc
namespace fsm {
class Visualizer {
 public:
  virtual std::string Visualize(FSM &) noexcept(false) = 0;
};

class MermaidVisualizer : public Visualizer {
 public:
  std::string Visualize(FSM &machine) noexcept(false) override {
	// we sort the keys alphabetically to have a reproducible graph output
	std::vector<impl::EKey> sorted_e_keys{};

	for (const auto &kv:machine.transitions_)
	  sorted_e_keys.push_back(kv.first);

	std::sort(sorted_e_keys.begin(), sorted_e_keys.end(), [](const impl::EKey &lhs, const impl::EKey &rhs) {
	  return lhs.src_ < rhs.src_;
	});

	std::stringstream buffer{};

	buffer << "graph TD" << std::endl;

	for (const auto &key:sorted_e_keys) {
	  auto val = machine.transitions_[key];
	  buffer << "    " << key.src_ << " -->|" << key.event_ << "| " << val << std::endl;
	}

	return buffer.str();
  }
};

class GraphvizVisualizer : public Visualizer {
 public:
  std::string Visualize(FSM &machine) noexcept(false) override {
	std::stringstream buffer{};
	std::map<std::string, int> states{};

	// we sort the keys alphabetically to have a reproducible graph output
	std::vector<impl::EKey> sorted_e_keys{};

	for (const auto &kv:machine.transitions_)
	  sorted_e_keys.push_back(kv.first);

	std::sort(sorted_e_keys.begin(), sorted_e_keys.end(), [](const impl::EKey &lhs, const impl::EKey &rhs) {
	  return lhs.src_ < rhs.src_;
	});

	buffer << "digraph fsm {" << std::endl;

	// make sure the initial state is at top
	for (const auto &key:sorted_e_keys) {
	  auto val = machine.transitions_[key];
	  if (key.src_ == machine.current_) {
		states[key.src_]++;
		states[val]++;
		buffer << "    \"" << key.src_ << "\" -> \"" << val << "\" [ label = \"" << key.event_ << "\" ];" << std::endl;
	  }
	}

	for (const auto &key:sorted_e_keys) {
	  auto val = machine.transitions_[key];
	  if (key.src_ != machine.current_) {
		states[key.src_]++;
		states[val]++;
		buffer << "    \"" << key.src_ << "\" -> \"" << val << "\" [ label = \"" << key.event_ << "\" ];" << std::endl;
	  }
	}

	buffer << std::endl;

	std::vector<std::string> sorted_state_keys{};

	for (const auto &kv:states)
	  sorted_state_keys.push_back(kv.first);

	std::sort(sorted_state_keys.begin(), sorted_state_keys.end());

	for (const auto &key:sorted_state_keys)
	  buffer << "    \"" << key << "\";" << std::endl;

	buffer << "}";

	return buffer.str();
  }
};

FSM::FSM(FSM const &fsm) :
	state_mu_(), event_mu_(),
	current_(fsm.current_),
	transitions_(fsm.transitions_),
	callbacks_(fsm.callbacks_) {}

VisualizeResult Visualize(FSM &machine, VisualizeType visualize_type) noexcept(false) {
  switch (visualize_type) {
	case VisualizeType::kGraphviz:
	  return std::make_pair(GraphvizVisualizer().Visualize(machine),
							std::optional<std::shared_ptr<Error>>{});
	case VisualizeType::kMermaid:
	  return std::make_pair(MermaidVisualizer().Visualize(machine),
							std::optional<std::shared_ptr<Error>>{});
	default: return VisualizeResult{"", {std::make_shared<UnknownVisualizeTypeError>()}};
  }
}
}

#endif //FSM__FSM_H_
