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

#include "errors.h"
#include "events.h"
#include "utils.h"

class FSMTestGroup;

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
#endif //FSM__FSM_H_
