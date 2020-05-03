//
// Created by fanghr on 2020/5/2.
//

#ifndef FSM_INCLUDE_FSM_FSM_H_
#define FSM_INCLUDE_FSM_FSM_H_

namespace fsm {
struct Event {
  virtual ~Event() = default;
};

namespace internal {
template<typename T, T v>
struct IntegralConstant {
  static constexpr T value = v;
  using ValueType = T;
  using Type = IntegralConstant;
  constexpr explicit operator ValueType() const noexcept { return value; }
};

using TrueType = IntegralConstant<bool, true>;
using FalseType = IntegralConstant<bool, false>;


template<typename T, typename U>
struct IsSame : FalseType {};

template<typename T>
struct IsSame<T, T> : TrueType {};

#ifdef DEBUG
template<typename F_1, typename F_2>
struct IsSameFSM { static constexpr bool value = true; };
#else
template<typename F_1, typename F_2>
struct IsSameFSM : IsSame<typename F_1::FSMType, typename F_2::FSMType> {};
#endif

template<typename S>
struct StateInstance {
  using ValueType = S;
  using Type = StateInstance<S>;
  static ValueType value;
};

template<typename S>
typename StateInstance<S>::ValueType  StateInstance<S>::value{};
}

template<typename F>
class FSM {
 public:
  using FSMType = FSM<F>;
  using StatePtrType = F *;

  static StatePtrType current_state_ptr_;

  template<typename S>
  static constexpr S &State() {
    static_assert(internal::IsSameFSM<F, S>::value, "accessing state of different state machine");
    return internal::StateInstance<S>::value;
  }

  template<typename S>
  static constexpr bool IsInState() {
    static_assert(internal::IsSameFSM<F, S>::value, "accessing state of different state machine");
    return current_state_ptr_ == &internal::StateInstance<S>::value;
  }
 public:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NotImplementedFunctions"
  static void SetInitialState(); // explicitly specialized in FSM_INITIAL_STATE
#pragma clang diagnostic pop

  static void Reset() { current_state_ptr_->Reset(); }

  static void Enter() { current_state_ptr_->Entry(); }

  static void Start() {
    SetInitialState();
    Enter();
  }

  template<typename E>
  static void Dispatch(const E &event) { current_state_ptr_->React(event); }

 protected:
  template<typename S>
  void Transit() {
    static_assert(internal::IsSameFSM<F, S>::value, "transit to different state machine");
    current_state_ptr_->Exit();
    current_state_ptr_ = &internal::StateInstance<S>::value;
    current_state_ptr_->Entry();
  }

  template<typename S, typename ActionFunction>
  void Transit(ActionFunction action_function) {
    static_assert(internal::IsSameFSM<F, S>::value, "transit to different state machine");
    current_state_ptr_->Exit();
    action_function();
    current_state_ptr_ = &internal::StateInstance<S>::value;
    current_state_ptr_->Entry();
  }

  template<typename S, typename ActionFunction, typename ConditionFunction>
  void Transit(ActionFunction action_function, ConditionFunction condition_function) {
    if (condition_function())
      Transit<S>(action_function);
  }
};

template<typename F>
typename FSM<F>::StatePtrType FSM<F>::current_state_ptr_{};

template<typename ...FF>
struct FSMList;

template<>
struct FSMList<> {
  static void SetInitialState() {}
  static void Reset() {}
  static void Enter() {}
  static void Start() {}
  template<typename E>
  static void Dispatch(const E &) {}
};

template<typename F, typename ...FF>
struct FSMList<F, FF...> {
  using FSMType = FSM<F>;

  static void SetInitialState() {
    FSMType::SetInitialState();
    FSMList<FF...>::SetInitialState();
  }

  static void Reset() {
    FSMType::Reset();
    FSMList<FF...>::Reset();
  }

  static void Enter() {
    FSMType::Enter();
    FSMList<FF...>::Enter();
  }

  static void Start() {
    FSMType::Start();
    FSMList<FF...>::Start();
  }

  template<typename E>
  static void Dispatch(const E &event) {
    FSMType::Dispatch(event);
    FSMList<FF...>::Dispatch(event);
  }
};

template<typename ...SS>
struct StateList;

template<>
struct StateList<> {
  static void Reset() {}
};

template<typename S, typename ...SS>
struct StateList<S, SS...> {
  static void Reset() {
    internal::StateInstance<S>::value = S();
    StateList<SS...>::Reset();
  }
};

template<typename F>
struct MooreMachine : FSM<F> {
  virtual void Entry() {}
  void Exit() {}
};

template<typename F>
struct MealyMachine : FSM<F> {
  void Entry() {}
  void Exit() {}
};
}

#define FSM_INITIAL_STATE(_FSM, _STATE)                             \
namespace fsm{                                                      \
template<> void FSM< _FSM >::SetInitialState() {                    \
  current_state_ptr_ = &internal::StateInstance< _STATE >::value;   \
}                                                                   \
}

#endif //FSM_INCLUDE_FSM_FSM_H_
