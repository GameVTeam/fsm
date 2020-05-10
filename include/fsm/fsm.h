// Copyright 2020 方泓睿
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// 	documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef FSM_INCLUDE_FSM_FSM_H_
#define FSM_INCLUDE_FSM_FSM_H_

//#if __cplusplus < 201103L
//#error "FSM requires C++11 support."
//#endif

#include <type_traits> // for std::result_of

#if defined(__cpp_exceptions)
#include <stdexcept> // for std::logic_error
#endif

// TODO remove all STL dependencies to support *Arduino*
//  Current usages of types/functions in STL (mainly from <type_traits>):
//  ❌ std::result_of (totally impossible to implement. Even partial.)
//  ✅ (fsm::detail::IntegralConstant) std::integral_constant
//  ✅ (fsm::detail::RemoveReference) std::remove_reference
//  ✅ (fsm::detail::Conditional) std::conditional
//  ✅ (fsm::detail::NullPointerType) std::nullptr_t
//  ✅ (fsm::detail::IsSame) std::is_same
//  ✅ (fsm::detail::IsBaseOf) std::is_base_of
//  ✅ (<partial implemented> fsm::detail::Forward) std::forward
//  ✅ (<partial implemented> fsm::detail::Invoke) std::invoke
//  ✅ (<conditional compilation>) std::logic_error (if exception is enabled)
//

namespace fsm {
namespace detail {
// TODO correctly implement ResultOf
//template<typename T>
//struct ResultOf;
//
//template<typename Ret>
//struct ResultOf<Ret (*)()> {
//  using Type = Ret;
//};
//
//template<typename Ret, typename T1>
//struct ResultOf<Ret (*)(T1)> {
//  using Type = Ret;
//};
//
//template<typename Ret, typename T1>
//struct ResultOf<Ret (*)(T1, ...)> {
//  using Type = Ret;
//};
//
//template<typename Ret, typename T1, typename T2>
//struct ResultOf<Ret (*)(T1, T2)> {
//  using Type = Ret;
//};
//
//template<typename Ret, typename T1, typename T2, typename T3>
//struct ResultOf<Ret (*)(T1, T2, T3)> {
//  using Type = Ret;
//};

template<bool B, class T, class F>
struct Conditional {
  using Type = T;
};

template<class T, class F>
struct Conditional<false, T, F> { using Type = F; };

using NullPointerType = decltype(nullptr);

template<class T, T v>
struct IntegralConstant {
  static constexpr T value = v;
  using ValueType = T;
  using Type = IntegralConstant;
  constexpr explicit operator ValueType() const noexcept { return value; }
};

using TrueType = IntegralConstant<bool, true>;
using FalseType = IntegralConstant<bool, false>;

template<class T, class U>
struct IsSame : FalseType {};

template<class T>
struct IsSame<T, T> : TrueType {};

template<typename B>
constexpr TrueType TestPrePtrConvertible(const volatile B *) { return TrueType{}; }

template<typename>
constexpr FalseType TestPrePtrConvertible(const volatile void *) { return FalseType{}; }

template<typename, typename>
constexpr TrueType TestPreIsBaseOf(...) { return TrueType{}; }

template<typename B, typename D>
constexpr decltype(TestPrePtrConvertible<B>(static_cast<D *>(nullptr))) TestPreIsBaseOf(int) {
  return decltype(TestPrePtrConvertible<B>(static_cast<D *>(nullptr))){};
}

// TODO implement IsClass
//  Note that currently it's impossible to implement std::is_union using template
//  (need compiler support), so we implement IsClassOrUnion instead of IsClass,
//  though IsClassOrUnionTest is enough for IsBaseOf (drop in replacement for std::is_base_of).
struct IsClassOrUnionTest {
  template<class T>
  static constexpr IntegralConstant<bool, /*!std::is_union<T>::value*/ true> Test(int T::*) {
    return IntegralConstant<bool, /*!std::is_union<T>::value*/ true>{};
  }

  template<class T>
  static constexpr FalseType Test(...) { return FalseType{}; }
};

using IsClassTest = IsClassOrUnionTest;

template<class T>
struct IsClassOrUnion : decltype(IsClassOrUnionTest::Test<T>(nullptr)) {};

template<class T>
using IsClass = IsClassOrUnion<T>;

template<typename Base, typename Derived>
struct IsBaseOf : IntegralConstant<bool, IsClass<Base>::value
    && IsClass<Derived>::value &&
    decltype(TestPreIsBaseOf<Base, Derived>(0))::value> {
};

template<typename T>
struct RemoveReference;

template<typename T>
struct RemoveReference<T &&> {
  using Type = T;
};

template<typename T>
struct RemoveReference<T &> {
  using Type = T;
};

template<class F, class... Args>
using InvokeResultType = typename std::result_of<F &&(Args &&...)>::type;

struct IsInvocableTest {
  struct NoType { int a; int b; }; // sizeof(NoType) != 1

  template<class F, class... Args, class = InvokeResultType<F, Args...>>
  static constexpr char Test(int) { return 0; }

  template<class, class...>
  static constexpr NoType Test(...) { return NoType{}; }
};

template<class F, class... Args>
using IsInvocableType = typename IntegralConstant<
    bool,
    sizeof(IsInvocableTest::Test<F, Args...>(0)) == 1
>::Type;

// C++11 `std::forward()` may not be present on freestanding implementations.
template<class T>
constexpr T &&Forward(typename RemoveReference<T>::Type &t) noexcept {
  return static_cast<T &&>(t);
}

template<class T>
constexpr T &&Forward(typename RemoveReference<T>::Type &&t) noexcept {
  return static_cast<T &&>(t);
}

// Partial implementation of C++17 `std::invoke`.
template<class F, class... Args>
InvokeResultType<F, Args...> Invoke(F &&f, Args &&... args) {
  return f(args...);
}

template<class M, class T, class T1, class... Args>
InvokeResultType<M T::*, T1, Args...> Invoke(M T::* f, T1 &&obj, Args &&... args) {
  return (obj.*f)(args...);
}

// F(), F(Arg1), F(Arg2) or F(Arg1, Arg2)
template<
    class F, class Arg1, class Arg2,
    bool f1 = IsInvocableType<F>::value,
    bool f2 = IsInvocableType<F, Arg1>::value,
    bool f3 = IsInvocableType<F, Arg2>::value,
    bool f4 = IsInvocableType<F, Arg1, Arg2>::value
>
struct BinaryFnHelper;

template<class F, class Arg1, class Arg2>
struct BinaryFnHelper<F, Arg1, Arg2, true, false, false, false> {
  using ResultType = InvokeResultType<F>;

  static ResultType Invoke(F &&f, Arg1 &&, Arg2 &&) {
    return detail::Invoke(f);
  }
};

template<class F, class Arg1, class Arg2>
struct BinaryFnHelper<F, Arg1, Arg2, false, true, false, false> {
  using ResultType = InvokeResultType<F, Arg1>;

  static ResultType Invoke(F &&f, Arg1 &&a, Arg2 &&) {
    return detail::Invoke(f, a);
  }
};

template<class F, class Arg1, class Arg2>
struct BinaryFnHelper<F, Arg1, Arg2, false, false, true, false> {
  using ResultType = InvokeResultType<F, Arg2>;

  static ResultType Invoke(F &&f, Arg1 &&, Arg2 &&b) {
    return detail::Invoke(f, b);
  }
};

template<class F, class Arg1, class Arg2>
struct BinaryFnHelper<F, Arg1, Arg2, false, false, false, true> {
  using ResultType = InvokeResultType<F, Arg1, Arg2>;

  static ResultType Invoke(F &&f, Arg1 &&a, Arg2 &&b) {
    return detail::Invoke(f, a, b);
  }
};

template<class F, class Arg1, class Arg2>
using InvokeAsBinaryFnResultType = typename BinaryFnHelper<F, Arg1, Arg2>::ResultType;

template<class F, class Arg1, class Arg2>
InvokeAsBinaryFnResultType<F, Arg1, Arg2> InvokeAsBinaryFn(F &&f, Arg1 &&a, Arg2 &&b) {
  return BinaryFnHelper<F, Arg1, Arg2>::Invoke(Forward<F>(f), Forward<Arg1>(a), Forward<Arg2>(b));
}

/**
 * Basic template metaprogramming stuff; note that we could use `std::tuple` instead,
 * but `std::tuple` may not be present on freestanding implementations.
 */
template<class...>
struct List {};

template<class...>
struct Concat;

template<class T, class... Types>
struct Concat<T, List<Types...>> {
  using Type = List<T, Types...>;
};

template<>
struct Concat<> {
  using Type = List<>;
};

template<template<typename> class Predicate, class...>
struct Filter;

template<template<typename> class Predicate, class T, class... Types>
struct Filter<Predicate, T, Types...> {
  using Type = typename detail::Conditional<
      Predicate<T>::value,
      typename Concat<T, typename Filter<Predicate, Types...>::Type>::Type,
      typename Filter<Predicate, Types...>::Type
  >::Type;
};

template<template<typename> class Predicate>
struct Filter<Predicate> {
  using Type = List<>;
};

class RWMutexInterface {
 public:
  virtual ~RWMutexInterface() = default;
  virtual void Lock() = 0;
  virtual void RLock() = 0;
  virtual void Unlock() = 0;
  virtual void RUnlock() = 0;

  template<bool is_read_lock>
  class Guard;

  template<>
  class Guard<false> {
    RWMutexInterface &mutex_;
   public:
    explicit Guard(RWMutexInterface &mutex) : mutex_(mutex) {
      mutex_.Lock();
    }
    ~Guard() {
      mutex_.Unlock();
    }
  };

  template<>
  class Guard<true> {
    RWMutexInterface &mutex_;
   public:
    explicit Guard(RWMutexInterface &mutex) : mutex_(mutex) {
      mutex_.RLock();
    }
    ~Guard() {
      mutex_.RUnlock();
    }
  };
};

class FakeRWMutex final : public RWMutexInterface {
 public:
  ~FakeRWMutex() override = default;
  void Lock() override {}
  void Unlock() override {}
  void RLock() override {}
  void RUnlock() override {}
};

using DefaultRWMutex = FakeRWMutex;
}

/**
 * Finite state machine (FSM) base class template.
 *
 * TODO use *atomic* bool instead of raw bool as default.
 * Note that this class *does not* guarantee thread safe by default.
 *
 * @tparam Derived the derived state machine class
 * @tparam State the FSM's state type, default to `int`
 * @tparam AtomicBoolType the atomic bool type to implement processing lock, default to `bool`
 */
template<class Derived, class State = int,
    class AtomicBoolType = bool,
    class MutexType = detail::DefaultRWMutex>
class FSM {
 public:
  /**
   * The FSM's state type.
   */
  using StateType = State;

 public:
  /**
   * Create a finite state machine with an optional initial state.
   *
   * @param initial_state the FSM's initial state.
   */
  explicit FSM(StateType init_state = StateType{}) : state_(init_state) {}

  /**
   * Dispatch an event.
   *
   * @warning This method shouldn't be called recursively.
   * @tparam Event the event type
   * @param event the optional event instance
   * @return FSM's state after dispatching the event.
   */
  template<class Event>
  StateType Dispatch(const Event &event = Event{}) {
    static_assert(detail::IsBaseOf<FSM, Derived>::value, "must derive from fsm");

    using rows = typename ByEventType<Event, typename Derived::TransitionTable>::Type;

    ProcessingLockGuard processing_lock_guard(*this);
    detail::RWMutexInterface::Guard<true> mutex_lock_guard(mutex_);

    auto &self = static_cast<Derived &>(*this);
    state_ = HandleEvent<Event, rows>::Execute(self, event, state_);

    return state_;
  }

  /**
   * Dispatch an event.
   *
   * @warning This method shouldn't be called recursively.
   * @tparam Event the event type
   * @param event the optional event instance
   * @return FSM's state after dispatching the event.
   */
  template<class Event>
  StateType operator()(const Event &event) {
    return Dispatch(event);
  }

  /**
   * Get current state of the state machine.
   *
   * @return current state
   */
  StateType CurrentState() const {
    detail::RWMutexInterface::Guard<false> mutex_lock_guard(mutex_);
    return state_;
  }

 protected:
  /**
   * Called when no transition can be found for the given event
   * in the current state. Derived state machine may override
   * this to throw an exception, or change to some other state.
   * The default is to return current state, so no state change occurs.
   *
   * @tparam Event
   * @param event
   * @return
   */
  template<class Event>
  StateType NoTransition(const Event &event) {
    return state_;
  }

 private:
  template<State start, class Event, State target>
  struct RowBase {
    using StateType = State;
    using EventType = Event;

    static constexpr StateType StartValue() { return start; }
    static constexpr StateType TargetValue() { return target; }

   protected:
    template<class Action>
    static void ProcessEvent(Action &&action, Derived &self, const Event &event) {
      detail::InvokeAsBinaryFn(action, self, event);
    }

    static /*constexpr*/ void ProcessEvent(detail::NullPointerType, Derived &self, const Event &event) {
    }

    template<class Guard>
    static bool CheckGuard(Guard &&guard, const Derived &self, const Event &event) {
      return detail::InvokeAsBinaryFn(guard, self, event);
    }

    static constexpr bool CheckGuard(detail::NullPointerType, const Derived &, const Event &) {
      return true;
    }
  };

 protected:
  /**
   * Transition table variadic class template.
   *
   * Each derived state machine class *should* define a nested
   * non-template type `TransitionTable` which is either derived from a
   * type or a type alias of Table.
   */
  template<class... Rows>
  using Table = detail::List<Rows...>;

  /**
   * Basic transition class template.
   *
   * @tparam start the start state of the transition
   * @tparam Event the event type of the transition
   * @tparam target the target state of the transition
   * @tparam Action a callable action function type or `detail::NullPointerType`
   * @tparam action a static `Action` instance
   * @tparam Guard a callable guard function type or `detail::NullPointerType`
   * @tparam guard a static `Guard` instance
   */
  template<
      State start,
      class Event,
      State target,
      class Action = detail::NullPointerType,
      Action action = nullptr,
      class Guard = detail::NullPointerType,
      Guard guard = nullptr
  >
  struct BasicRow : public RowBase<start, Event, target> {
    static void ProcessEvent(Derived &self, const Event &event) {
      RowBase<start, Event, target>::ProcessEvent(action, self, event);
    }

    static bool CheckGuard(const Derived &self, const Event &event) {
      return RowBase<start, Event, target>::CheckGuard(guard, self, event);
    }
  };

  /**
   * Basic transition class template.
   *
   * @tparam start the start state of the transition
   * @tparam Event the event type of the transition
   * @tparam target the target state of the transition
   * @tparam action an action member function or `nullptr`
   * @tparam guard a guard member function or `nullptr`
   */
  template<
      State start,
      class Event,
      State target,
      void (Derived::*action)(const Event &) = nullptr,
      bool (Derived::*guard)(const Event &) const = nullptr
  >
  struct MemFnRow : public RowBase<start, Event, target> {
    static void ProcessEvent(Derived &self, const Event &event) {
      if (action != nullptr)
        RowBase<start, Event, target>::ProcessEvent(action, self, event);
    }

    static bool CheckGuard(const Derived &self, const Event &event) {
      if (guard != nullptr)
        return RowBase<start, Event, target>::CheckGuard(guard, self, event);
      return true;
    }
  };

 private:
  template<class Event, class...>
  struct ByEventType;

  template<class Event, class... Types>
  struct ByEventType<Event, detail::List<Types...>> {
    template<class T> using Predicate = detail::IsSame<typename T::EventType, Event>;
    using Type = typename detail::Filter<Predicate, Types...>::Type;
  };

  template<class Event>
  struct ByEventType<Event, detail::List<>> {
    using Type = detail::List<>;
  };

  template<class Event, class...>
  struct HandleEvent;

  template<class Event, class T, class... Types>
  struct HandleEvent<Event, detail::List<T, Types...>> {
    static State Execute(Derived &self, const Event &event, State state) {
      if (state == T::StartValue() && T::CheckGuard(self, event)) {
        T::ProcessEvent(self, event);
        return T::TargetValue();
      }
      return HandleEvent<Event, detail::List<Types...>>::Execute(self, event, state);
    }
  };

  template<class Event>
  struct HandleEvent<Event, detail::List<>> {
    static State Execute(Derived &self, const Event &event, State) {
      return self.NoTransition(event);
    }
  };

 private:
  StateType state_;

 private:
  class ProcessingLockGuard {
   public:
    explicit ProcessingLockGuard(FSM &m) : processing_(m.processing_) {
      if (processing_) {
#if defined(__cpp_exceptions)
        throw std::logic_error("trying to lock processing lock recursively");
#else
#if __has_include(<cstdlib>) // for exit()
#include <cstdlib>
#else
#include <stdlib.h>
#endif
        exit(EXIT_FAILURE);
#endif
      }
      processing_ = true; // TODO atomically compare && replace
    }

    ~ProcessingLockGuard() {
      processing_ = false; // TODO atomically compare && replace
    }
   private:
    AtomicBoolType &processing_;
  };

  AtomicBoolType processing_ = false;

 private:
  mutable MutexType mutex_{};
};
}

#endif //FSM_INCLUDE_FSM_FSM_H_
