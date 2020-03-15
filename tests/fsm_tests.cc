//
// Created by 方泓睿 on 2020/3/7.
//

#include <thread>
#include <atomic>

#include <gtest/gtest.h>

#include <fsm/fsm.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

class FSMTestGroup : public testing::Test {};

TEST_F(FSMTestGroup, TestSameState) {
  fsm::FSM machine(
	  "start",
	  {
		  {"run", {"start"}, "start"}
	  }, {});
  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestSetState) {
  fsm::FSM machine("walking", {{"walk", {"start"}, "walking"}}, {});
  machine.SetState("start");
  if (machine.Current() != "start")
	FAIL() << "expected state to be walking";
  auto err = machine.FireEvent("walk");
  if (err)
	FAIL() << "transition is expected no error";
}

TEST_F(FSMTestGroup, TestInappropriateEvent) {
  fsm::FSM machine("closed", {
	  {"open", {"closed"}, "open"},
	  {"close", {"open"}, "closed"},
  }, {});
  auto err = machine.FireEvent("close");
  if (!err || !std::dynamic_pointer_cast<fsm::InvalidEventError>(err))
	FAIL() << "expected 'InvalidEventError' with correct state and event";
}

TEST_F(FSMTestGroup, TestInvalidEvent) {
  fsm::FSM machine("closed", {
	  {"open", {"closed"}, "open"},
	  {"close", {"open"}, "closed"},
  }, {});
  auto err = machine.FireEvent("lock");
  if (!err || !std::dynamic_pointer_cast<fsm::UnknownEventError>(err))
	FAIL() << "expected 'UnknownEventError' with correct event";
}

TEST_F(FSMTestGroup, TestMultipleSources) {
  fsm::FSM machine("one",
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
  fsm::FSM machine("start",
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

  fsm::FSM machine("start",
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

  fsm::FSM machine("start",
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

  fsm::FSM machine(
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

  fsm::FSM machine(
	  "start",
	  {{"do_not_run", {"start"}, "start"}},
	  {{"before_event", [&](fsm::Event &event) { before_event = true; }}}
  );

  auto err = machine.FireEvent("do_not_run");
  if (!err || !std::dynamic_pointer_cast<fsm::NoTransitionError>(err))
	FAIL() << "expected 'NoTransitionError' without custom error";

  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";

  if (!before_event)
	FAIL() << "expected callback to be called once";
}

TEST_F(FSMTestGroup, TestCancelBeforeGenericEvent) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"before_event", [&](fsm::Event &event) { event.Cancel(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestCancelBeforeSpecificEvent) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"before_run", [&](fsm::Event &event) { event.Cancel(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestCancelLeaveGenericState) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_state", [&](fsm::Event &event) { event.Cancel(); }}});

  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestCancelLeaveSpecificState) {
  fsm::FSM machine(
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
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_start", [&](fsm::Event &event) { event.Cancel({std::make_shared<FakeError>()}); }}});

  auto err = machine.FireEvent("run");

  if (!err || !std::dynamic_pointer_cast<fsm::CanceledError>(err))
	FAIL() << "expected only 'CanceledError'";

  if (!std::dynamic_pointer_cast<FakeError>(std::dynamic_pointer_cast<fsm::CanceledError>(err)->error_))
	FAIL() << "expected 'CanceledError' with correct custom error";

  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestAsyncTransitionGenericState) {
  fsm::FSM machine(
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
  fsm::FSM machine(
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
  fsm::FSM machine(
	  "start",
	  {
		  {"run", {"start"}, "end"},
		  {"reset", {"end"}, "start"},
	  },
	  {
		  {"leave_start", [&](fsm::Event &event) { event.Async(); }}
	  });
  machine.FireEvent("run");
  auto err = machine.FireEvent("reset");

  if (!err ||
	  !std::dynamic_pointer_cast<fsm::InTransitionError>(err) ||
	  std::dynamic_pointer_cast<fsm::InTransitionError>(err)->event_ != "reset")
	FAIL() << "expected state to be start";

  machine.Transition();
  machine.FireEvent("reset");
  if (machine.Current() != "start")
	FAIL() << "expected state to be 'start'";
}

TEST_F(FSMTestGroup, TestAsyncTransitionNotInprogress) {
  fsm::FSM machine(
	  "start",
	  {
		  {"run", {"start"}, "end"},
		  {"reset", {"end"}, "start"},
	  },
	  {});

  auto err = machine.Transition();
  if (!err || !std::dynamic_pointer_cast<fsm::NotInTransitionError>(err))
	FAIL() << "expected 'NotInTransitionError'";
}

TEST_F(FSMTestGroup, TestCallbackNoError) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"run", [&](fsm::Event &) {}}});

  auto err = machine.FireEvent("run");
  if (err)
	FAIL() << "expected no error";
}

TEST_F(FSMTestGroup, TestCallbackError) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"run", [&](fsm::Event &event) { event.error_ = {std::make_shared<FakeError>()}; }}});

  auto err = machine.FireEvent("run");

  if (!err || !std::dynamic_pointer_cast<FakeError>(err))
	FAIL() << "expected error to be 'FakeError'";
}

TEST_F(FSMTestGroup, TestCallbackArgs) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"run", [&](fsm::Event &event) {
		if (event.args_.size() != 1)
		  FAIL() << "expected args size to be 1";
		std::string arg{};
		try {
		  arg = std::any_cast<std::string>(event.args_[0]);
		} catch (...) {
		  FAIL() << "not a string argument";
		}
		if (arg != "test")
		  FAIL() << "incorrect argument";
	  }}});

  machine.FireEvent("run", {std::string("test")});
}

TEST_F(FSMTestGroup, TestNoDeadLock) {
  fsm::FSM *machine;
  machine = new fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"run", [&](fsm::Event &) {
		machine->Current(); // should not result in dead lock
	  }}});

  machine->FireEvent("run");

  delete machine;
}

TEST_F(FSMTestGroup, TestThreadSafteyRaceCondition) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"run", [&](fsm::Event &) {}}});

  auto thread = std::thread([&]() {
	machine.Current();
  });

  // TODO complex condition

  auto err = machine.FireEvent("run");

  thread.join();

  if (err)
	FAIL() << "not thread safety";
}

TEST_F(FSMTestGroup, TestDoubleTransition) {
  fsm::FSM *machine;

  std::atomic<int> wait_group(2);

  machine = new fsm::FSM(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {
		  {
			  "before_run", [&](fsm::Event &event) {

			wait_group--;
			// Imagine a concurrent event coming in of the same type while
			// the data access mutex is unlocked because the current transition
			// is running is event callbacks, getting around the "active"
			// transition checks.
			if (event.args_.empty()) {
			  // Must be concurrent so the test may pass when we add a mutex that synchronizes
			  // calls to Event(...). It will then fail as an inappropriate transition as we
			  // have changed state.
			  std::thread([&]() {
				machine->FireEvent("run", {"second run"}); // should fail
				wait_group--;
			  }).detach();
			} else {
			  FAIL() << "was able to reissue an event mid-transition";
			}
		  }
		  }
	  }
  );

  machine->FireEvent("run");

  while (wait_group);

  delete machine;
}

TEST_F(FSMTestGroup, TestNoTransition) {
  fsm::FSM machine("start",
				   {{"run", {"start"}, "start"}});

  auto err = machine.FireEvent("run");
  if (!err || !std::dynamic_pointer_cast<fsm::NoTransitionError>(err))
	FAIL() << "expected 'NoTransitionError'";
}

#pragma clang diagnostic pop