//
// Created by fanghr on 2020/5/10.
//

#include "fsm_test.h"

namespace {
class StateMachine : public fsm::FSM<StateMachine> {
  friend class FSM;
 public:
  enum States { kInit, kExit };
  using Event = int;

  void DispatchEvent(const Event &event) {
    Dispatch(event + 1);
  }
 private:
  using TransitionTable = Table<MemFnRow<kInit, Event, kExit, &StateMachine::DispatchEvent>>;
};
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

TEST_F(FSMTestSuite, TestRecursive) {
  StateMachine machine{};
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  EXPECT_THROW(machine(1), std::logic_error);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  EXPECT_THROW(machine(1), std::logic_error);
}

#pragma clang diagnostic pop