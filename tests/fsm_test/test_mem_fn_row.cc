//
// Created by 方泓睿 on 2020/5/9.
//

#include "fsm_test.h"

namespace {
class StateMachine : public fsm::FSM<StateMachine> {
  friend class FSM;

 public:
  enum States { kInit, kEven, kOdd };

  using Event = int;

 private:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"
  bool IsEven(const Event &event) const {
    return event % 2 == 0;
  }

  bool IsOdd(const Event &event) const {
    return event % 2 != 0;
  }
#pragma clang diagnostic pop

 private:
  using TransitionTable = Table<
  MemFnRow<kInit, Event, kEven, nullptr, &StateMachine::IsEven>,
  MemFnRow<kInit, Event, kOdd, nullptr, &StateMachine::IsOdd>,
  MemFnRow<kEven, Event, kOdd, nullptr, &StateMachine::IsOdd>,
  MemFnRow<kOdd, Event, kEven, nullptr, &StateMachine::IsEven>,
  MemFnRow<kOdd, Event, kOdd, nullptr, &StateMachine::IsOdd>,
  MemFnRow<kEven, Event, kEven>>;
};
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

TEST_F(FSMTestSuite, TestMemFnRowEven) {
  StateMachine machine{};
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  machine(0);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kEven);
  machine(0);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kEven);
  machine(1);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kOdd);
}

TEST_F(FSMTestSuite, TestMemFnRowOdd) {
  StateMachine machine{};
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  machine(1);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kOdd);
  machine(1);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kOdd);
  machine(0);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kEven);
}
#pragma clang diagnostic pop