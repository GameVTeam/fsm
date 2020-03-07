//
// Created by 方泓睿 on 2020/3/7.
//

#include <gtest/gtest.h>

#include <fsm/fsm.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

class FSMTestGroup : public testing::Test {};

TEST_F(FSMTestGroup, TestSameState) {
  auto machine = fsm::FSM(
	  "start",
	  {
		  {"run", {"start"}, "start"}
	  }, {});
  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestSetState) {
  auto machine = fsm::FSM("walking", {{"walk", {"start"}, "walking"}}, {});
  machine.SetState("start");
  if (machine.Current() != "start")
	FAIL() << "expected state to be walking";
  auto err = machine.FireEvent("walk");
  if (err)
	FAIL() << "transition is expected no error";
}

TEST_F(FSMTestGroup, TestInappropriateEvent) {
  auto machine = fsm::FSM("closed", {
	  {"open", {"closed"}, "open"},
	  {"close", {"open"}, "closed"},
  }, {});
  auto err = machine.FireEvent("close");
  if (!err || !std::dynamic_pointer_cast<fsm::InvalidEventError>(err.value()))
	FAIL() << "expected 'InvalidEventError' with correct state and event";
}

TEST_F(FSMTestGroup, TestInvalidEvent) {
  auto machine = fsm::FSM("closed", {
	  {"open", {"closed"}, "open"},
	  {"close", {"open"}, "closed"},
  }, {});
  auto err = machine.FireEvent("lock");
  if (!err || !std::dynamic_pointer_cast<fsm::UnknownEventError>(err.value()))
	FAIL() << "expected 'UnknownEventError' with correct event";
}

TEST_F(FSMTestGroup, TestMultipleSources) {
  auto machine = fsm::FSM("one",
						  {
							  {"first", {"one"}, "two"},
							  {"second", {"two"}, "three"},
							  {"reset", {"one", "two", "three"}, "one"},
						  }, {});

  auto err = machine.FireEvent("first");
  if (err || machine.Current() != "two")
	FAIL() << "expected state to be 'two'";

  err = machine.FireEvent("reset");
  if (err || machine.Current() != "one")
	FAIL() << "expected state to be 'one'";

  machine.FireEvent("first");
  err = machine.FireEvent("second");
  if (err || machine.Current() != "three")
	FAIL() << "expected state to be three";

  err = machine.FireEvent("reset");
  if (err || machine.Current() != "one")
	FAIL() << "expected state to be 'one'";
}

TEST_F(FSMTestGroup, TestMultipleEvents) {
  auto machine = fsm::FSM("start",
						  {
							  {"first", {"start"}, "one"},
							  {"second", {"start"}, "two"},
							  {"reset", {"one"}, "reset_one"},
							  {"reset", {"two"}, "reset_two"},
							  {"reset", {"reset_one", "reset_two"}, "start"},
						  }, {});

  machine.FireEvent("first");
  auto err = machine.FireEvent("reset");
  if (err || machine.Current() != "reset_one")
	FAIL() << "expected state to be 'reset_one'";

  err = machine.FireEvent("reset");
  if (err || machine.Current() != "start")
	FAIL() << "expected state to be 'start'";

  machine.FireEvent("second");
  err = machine.FireEvent("reset");
  if (err || machine.Current() != "reset_two")
	FAIL() << "expected state to be 'reset_one'";

  err = machine.FireEvent("reset");
  if (err || machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestGenericCallbacks) {
  bool before_event{}, leave_state{}, enter_state{}, after_event{};

  auto machine = fsm::FSM("start",
						  {{"run", {"start"}, "end"},},
						  {
							  {"before_event", [&](fsm::Event &event) { before_event = !before_event; }},
							  {"leave_state", [&](fsm::Event &event) { leave_state = !leave_state; }},
							  {"enter_state", [&](fsm::Event &event) { enter_state = !enter_state; }},
							  {"after_event", [&](fsm::Event &event) { after_event = !after_event; }}
						  });
  auto err = machine.FireEvent("run");
  if (err || !(before_event && leave_state && enter_state && after_event))
	FAIL() << "expected all callbacks to be called once";
}

TEST_F(FSMTestGroup, TestSpecificCallbacks) {
  bool before_event{}, leave_state{}, enter_state{}, after_event{};

  auto machine = fsm::FSM("start",
						  {{"run", {"start"}, "end"},},
						  {
							  {"before_run", [&](fsm::Event &) { before_event = !before_event; }},
							  {"leave_start", [&](fsm::Event &) { leave_state = !leave_state; }},
							  {"enter_end", [&](fsm::Event &) { enter_state = !enter_state; }},
							  {"after_run", [&](fsm::Event &) { after_event = !after_event; }}
						  });

  auto err = machine.FireEvent("run");
  if (err || !(before_event && leave_state && enter_state && after_event))
	FAIL() << "expected all callbacks to be called once";
}

TEST_F(FSMTestGroup, TestSpecificCallbacksShortform) {
  bool enter_state{}, after_event{};

  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {
		  {"end", [&](fsm::Event &) { enter_state = !enter_state; }},
		  {"run", [&](fsm::Event &) { after_event = !after_event; }}
	  });

  auto err = machine.FireEvent("run");
  if (err || !(enter_state && after_event))
	FAIL() << "expected all callbacks to be called once";
}

TEST_F(FSMTestGroup, TestBeforeEventWithoutTransition) {
  auto before_event = true;

  auto machine = fsm::FSM(
	  "start",
	  {{"do_not_run", {"start"}, "start"}},
	  {{"before_event", [&](fsm::Event &event) { before_event = true; }}}
  );

  auto err = machine.FireEvent("do_not_run");
  if (!err || !std::dynamic_pointer_cast<fsm::NoTransitionError>(err.value()))
	FAIL() << "expected 'NoTransitionError' without custom error";

  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";

  if (!before_event)
	FAIL() << "expected callback to be called once";
}

TEST_F(FSMTestGroup, TestCancelBeforeGenericEvent) {
  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"before_event", [&](fsm::Event &event) { event.Cancel(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestCancelBeforeSpecificEvent) {
  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"before_run", [&](fsm::Event &event) { event.Cancel(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestCancelLeaveGenericState) {
  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_state", [&](fsm::Event &event) { event.Cancel(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestCancelLeaveSpecificState) {
  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_start", [&](fsm::Event &event) { event.Cancel(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

class FakeError : public fsm::Error {
 public:
  [[nodiscard]] std::string What() const noexcept(false) override {
	return "error";
  }
};

TEST_F(FSMTestGroup, TestCancelWithError) {
  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_start", [&](fsm::Event &event) { event.Cancel({std::make_shared<FakeError>()}); }}});

  auto err = machine.FireEvent("run");

  if (!err || !std::dynamic_pointer_cast<fsm::CanceledError>(err.value()))
	FAIL() << "expected only 'CanceledError'";

  if (!std::dynamic_pointer_cast<FakeError>(std::dynamic_pointer_cast<fsm::CanceledError>(err.value())->error_))
	FAIL() << "expected 'CanceledError' with correct custom error";

  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestAsyncTransitionGenericState) {
  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_state", [&](fsm::Event &event) { event.Async(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";

  machine.Transition();

  if (machine.Current() != "end")
	FAIL() << "expected to be 'end'";
}

TEST_F(FSMTestGroup, TestAsyncTransitionSpecificState) {
  auto machine = fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_start", [&](fsm::Event &event) { event.Async(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";

  machine.Transition();

  if (machine.Current() != "end")
	FAIL() << "expected to be 'end'";
}

TEST_F(FSMTestGroup, TestAsyncTransitionInProgress) {
  auto machine = fsm::FSM(
	  "start",
	  {
		  {"run", {"start"}, "end"},
		  {"reset", {"end"}, "start"},
	  },
	  {
		  {"leave_start", [&](fsm::Event &event) { event.Async(); }}
	  });

}

#pragma clang diagnostic pop