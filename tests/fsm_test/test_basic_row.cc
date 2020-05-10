
//
// Created by fanghr on 2020/5/10.
//

#include "fsm_test.h"

namespace {
int kValue = 0;

void Store(int i) { kValue = i; }
void Clear() { kValue = 0; }

bool IsOne(int i) { return i == 1; }

class StateMachine : public fsm::FSM<StateMachine> {
  friend class FSM;
 public:
  enum States { kInit, kRunning, kExit };

  using Event = int;
 public:
  StateMachine() : fsm::FSM<StateMachine>(kInit) {}

  static void StoreTwo() { kValue = 2; }
  static bool IsTwo(int i) { return i == 2; }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

  void StoreThree() { kValue = 3; }
  bool IsThree(int i) const { return i == 3; }

#pragma clang diagnostic pop

 private:
  using TransitionTable = Table<
  BasicRow<kInit, Event, kRunning, decltype(&Store), &Store>,
  BasicRow<kRunning, Event, kRunning, decltype(&Store), &Store, decltype(&IsOne), &IsOne>,
  BasicRow<kRunning,
           Event,
           kRunning,
           decltype(&StateMachine::StoreTwo),
           &StateMachine::StoreTwo,
           decltype(&StateMachine::IsTwo),
           &StateMachine::IsTwo>,
  BasicRow<kRunning,
           Event,
           kRunning,
           decltype(&StateMachine::StoreThree),
           &StateMachine::StoreThree,
           decltype(&StateMachine::IsThree),
           &StateMachine::IsThree>,
  BasicRow<kRunning, Event, kExit, decltype(&Clear), &Clear/*fallback*/>,
  BasicRow<kExit, Event, kExit>>;
};
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

TEST_F(FSMTestSuite, TestBasicRow) {
  StateMachine machine{};
  EXPECT_EQ(machine.CurrentState(), StateMachine::kInit);
  machine(42);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kRunning);
  EXPECT_EQ(kValue, 42);
  machine(1);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kRunning);
  EXPECT_EQ(kValue, 1);
  machine(2);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kRunning);
  EXPECT_EQ(kValue, 2);
  machine(3);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kRunning);
  EXPECT_EQ(kValue, 3);
  machine(42);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kExit);
  EXPECT_EQ(kValue, 0);
  machine(42);
  EXPECT_EQ(machine.CurrentState(), StateMachine::kExit);
  EXPECT_EQ(kValue, 0);
}

#pragma clang diagnostic pop