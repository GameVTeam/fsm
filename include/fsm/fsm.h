//
// Created by 方泓睿 on 2020/3/5.
//

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
#include <shared_mutex>
#include <any>
#include <algorithm>
#include <sstream>
#include <map>

//#include "errors.h"
//#include "events.h"
//#include "utils.h"

// errors.h
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

// utils.h
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

// events.h
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

// fsm.h
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
  std::shared_mutex state_mu_{};

  class RLockGuard {
	std::shared_mutex &mu_;
   public:
	explicit RLockGuard(std::shared_mutex &mu) : mu_(mu) {
	  mu_.lock_shared();
	}
	~RLockGuard() {
	  mu_.unlock_shared();
	}
  };

  class WLockGuard {
	std::shared_mutex &mu_;
   public:
	explicit WLockGuard(std::shared_mutex &mu) : mu_(mu) {
	  mu_.lock();
	}
	~WLockGuard() {
	  mu_.unlock();
	}

  };

  class LockGuard {
	std::mutex &mu_;
   public:
	explicit LockGuard(std::mutex &mu) : mu_(mu) {
	  mu_.lock();
	}
	~LockGuard() {
	  mu_.unlock();
	}
  };

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

// events.cc
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

// fsm.cc
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

	auto helper = [&](std::string_view prefix) -> bool {
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
	return std::make_shared<InTransitionError>(event);

  auto iter = transitions_.find(impl::EKey{event, current_});

  if (iter == transitions_.end()) {
	for (const auto &ekv:transitions_)
	  if (ekv.first.event_ == event)
		return std::make_shared<InvalidEventError>(event, current_);
	return std::make_shared<UnknownEventError>(event);
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
	return std::make_shared<NoTransitionError>(event_obj->error_ ? event_obj->error_.value() : nullptr);
  }

  // Setup the transition, call it later.
  transition_ = [=]() {
	state_mu_.lock();
	current_ = dst;
	state_mu_.unlock();

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
  state_mu_.unlock_shared();
  err = DoTransition();
  state_mu_.lock_shared();

  if (err)
	return std::make_shared<InternalError>();

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
	  return std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr);
	else if (event.async_)
	  return std::make_shared<AsyncError>(event.error_ ? event.error_.value() : nullptr);
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kLeaveState
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr);
	else if (event.async_)
	  return std::make_shared<AsyncError>(event.error_ ? event.error_.value() : nullptr);
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
	  return std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr);
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kBeforeEvent
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return std::make_shared<CanceledError>(event.error_ ? event.error_.value() : nullptr);
  }
  return {};
}

VisualizeResult FSM::Visualize(VisualizeType visualize_type) noexcept(false) {
  return ::fsm::Visualize(*this, visualize_type);
}

namespace impl {
std::optional<std::shared_ptr<Error> > TransitionerClass::Transition(FSM &machine) noexcept(false) {
  if (!machine.transition_) {
	return std::make_shared<NotInTransitionError>();
  }
  machine.transition_();
  FSM::WLockGuard guard(machine.state_mu_);
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

// utils.cc
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

VisualizeResult Visualize(FSM &machine, VisualizeType visualize_type) noexcept(false) {
  switch (visualize_type) {
	case VisualizeType::kGraphviz:
	  return std::make_pair(GraphvizVisualizer().Visualize(machine),
							std::optional<std::shared_ptr<Error>>{});
	case VisualizeType::kMermaid:
	  return std::make_pair(MermaidVisualizer().Visualize(machine),
							std::optional<std::shared_ptr<Error>>{});
	default: return std::make_pair("", std::make_shared<UnknownVisualizeTypeError>());
  }
}
}
#endif //FSM__FSM_H_
