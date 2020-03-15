//
// Created by 方泓睿 on 2020/3/5.
//

#ifndef FSM__ERROR_H_
#define FSM__ERROR_H_

#include <string>
#include <utility>
#include <memory>

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
  
  virtual ~Error() = default;
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
#endif //FSM__ERROR_H_
