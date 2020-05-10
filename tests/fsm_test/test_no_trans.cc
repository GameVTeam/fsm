
//
// Created by fanghr on 2020/5/10.
//

#include "fsm_test.h"

namespace {
class StateMachine : public fsm::FSM<StateMachine> {
  friend class FSM;
 public:
  enum States { kInit, kExit, kError };

  struct Event {};
  struct Reset {};

 private:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"

  template<class EventType>
  StateType NoTransition(const EventType &) {
    return kError;
  }

  template<>
  StateType NoTransition<Reset>(const Reset &) {
    return kInit;
  }

#pragma clang diagnostic pop
 private:
  using TransitionTable = Table<BasicRow<kInit, Event, kExit>>;
};
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

TEST_F(FSMTestSuite, TestNoTrans) {
  StateMachine machine{};
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  machine(StateMachine::Reset{});
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  machine(StateMachine::Event{});
  EXPECT_EQ(machine.CurrentState(), StateMachine::kExit);
  machine(StateMachine::Reset{});
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  machine(StateMachine::Event{});
  EXPECT_EQ(machine.CurrentState(), StateMachine::kExit);
  machine(StateMachine::Event{});
  EXPECT_EQ(machine.CurrentState(), StateMachine::kError);
}

#pragma clang diagnostic pop