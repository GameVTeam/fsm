//
// Created by 方泓睿 on 2020/3/5.
//

#ifndef FSM__EVENT_H_
#define FSM__EVENT_H_

#include <utility>
#include <vector>
#include <functional>

#include "errors.h"
#include "any.h"

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
  std::shared_ptr<Error> error_{};

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
#endif //FSM__EVENT_H_
