//
// Created by fanghr on 2020/5/10.
//

#include "fsm_test.h"

namespace {
enum State { kInit, kExit };

class StateMachine : public fsm::FSM<StateMachine> {
  friend class FSM;
 public:
  struct Event {};

 private:
  using TransitionTable = Table<BasicRow<kInit, Event, kExit>>;
};
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

TEST_F(FSMTestSuite, TestScoped) {
  StateMachine machine{};
  EXPECT_EQ(machine.CurrentState(), kInit);
  machine(StateMachine::Event{});
  EXPECT_EQ(machine.CurrentState(), kExit);
}

#pragma clang diagnostic pop