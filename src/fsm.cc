//
// Created by 方泓睿 on 2020/3/7.
//

#include "fsm/fsm.h"

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
  for (const auto &kv:callbacks)
	SetCallback(kv.first, kv.second);
}

FSM::FSM(FSM const &fsm) :
	state_mu_(), event_mu_(),
	current_(fsm.current_),
	transitions_(fsm.transitions_),
	callbacks_(fsm.callbacks_) {}

std::shared_ptr<Error> FSM::FireEvent(const std::string &event,
									  std::vector<std::any> args) noexcept(false) {
  LockGuard event_mu_guard(event_mu_);
  RLockGuard state_mu_guard(state_mu_);

  if (transition_)
	return {std::make_shared<InTransitionError>(event)};

  auto iter = transitions_.find(impl::EKey{event, current_});

  if (iter == transitions_.end()) {
	for (const auto &ekv:transitions_)
	  if (ekv.first.event_ == event)
		return {std::make_shared<InvalidEventError>(event, current_)};
	return {std::make_shared<UnknownEventError>(event)};
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
	return {std::make_shared<NoTransitionError>(event_obj->error_)};
  }

  // Setup the transition, call it later.
  transition_ = [=]() {
	state_mu_.Lock();
	current_ = dst;
	state_mu_.Unlock();

	EnterStateCallbacks(*event_obj);
	AfterEventCallbacks(*event_obj);
  };

  err = LeaveStateCallbacks(*event_obj);

  if (err) {
	if (std::dynamic_pointer_cast<CanceledError>(err) != nullptr)
	  transition_ = nullptr;
	return err;
  }

  // Perform the rest of the transition, if not asynchronous.
  state_mu_.Unlock();
  err = DoTransition();
  state_mu_.RLock();

  if (err)
	return {std::make_shared<InternalError>()};

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

std::shared_ptr<Error> FSM::Transition() noexcept(false) {
  LockGuard guard(event_mu_);
  return DoTransition();
}

std::shared_ptr<Error> FSM::DoTransition() noexcept(false) {
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

std::shared_ptr<Error> FSM::LeaveStateCallbacks(Event &event) noexcept(false) {
  auto iter = callbacks_.end();
  if ((iter = callbacks_.find(impl::CKey{
	  current_,
	  impl::CallbackType::kLeaveState
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_)};
	else if (event.async_)
	  return {std::make_shared<AsyncError>(event.error_)};
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kLeaveState
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_)};
	else if (event.async_)
	  return {std::make_shared<AsyncError>(event.error_)};
  }
  return {};
}

std::shared_ptr<Error> FSM::BeforeEventCallbacks(Event &event) noexcept(false) {
  auto iter = callbacks_.end();
  if ((iter = callbacks_.find(impl::CKey{
	  event.event_,
	  impl::CallbackType::kBeforeEvent
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_)};
  }
  if ((iter = callbacks_.find(impl::CKey{
	  "",
	  impl::CallbackType::kBeforeEvent
  })) != callbacks_.end()) {
	iter->second(event);
	if (event.canceled_)
	  return {std::make_shared<CanceledError>(event.error_)};
  }
  return {};
}

VisualizeResult FSM::Visualize(VisualizeType visualize_type) noexcept(false) {
  return ::fsm::Visualize(*this, visualize_type);
}

void FSM::SetCallback(const std::string &name, const Callback &callback) noexcept(false) {
  WLockGuard state_mu_guard(state_mu_);
  LockGuard event_mu_guard(event_mu_);

  enum class type {
	kNone,
	kEvent,
	kState
  };

  auto find = [&](const std::string &target) {
	for (const auto &kv:transitions_) {
	  if (kv.first.src_ == target || kv.second == target)
		return type::kState;
	  if (kv.first.event_ == target)
		return type::kEvent;
	}
	return type::kNone;
  };

  auto is_event = [&](const std::string &target) {
	return find(target) != type::kNone && find(target) != type::kState;
  };

  auto is_state = [&](const std::string &target) {
	return find(target) != type::kNone && find(target) != type::kEvent;
  };

  std::string target{};
  impl::CallbackType callback_type{};

#if __cplusplus >= 201703L
  auto helper = [&](std::string_view prefix) -> bool {
#else
  auto helper = [&](const std::string &prefix) -> bool {
#endif
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
	} else if (is_event(target))
	  callback_type = impl::CallbackType::kBeforeEvent;
  } else if (helper("leave_")) {
	if (target == "state") {
	  target = "";
	  callback_type = impl::CallbackType::kLeaveState;
	} else if (is_state(target))
	  callback_type = impl::CallbackType::kLeaveState;
  } else if (helper("enter_")) {
	if (target == "state") {
	  target = "";
	  callback_type = impl::CallbackType::kEnterState;
	} else if (is_state(target))
	  callback_type = impl::CallbackType::kEnterState;
  } else if (helper("after_")) {
	if (target == "event") {
	  target = "";
	  callback_type = impl::CallbackType::kAfterEvent;
	} else if (is_event(target))
	  callback_type = impl::CallbackType::kAfterEvent;
  } else {
	target = name;
	if (is_state(target))
	  callback_type = impl::CallbackType::kEnterState;
	else if (is_event(target))
	  callback_type = impl::CallbackType::kAfterEvent;
  }

  if (callback_type != impl::CallbackType::kNone)
	callbacks_[impl::CKey{target, callback_type}] = callback;
}

namespace impl {
std::shared_ptr<Error> TransitionerClass::Transition(FSM &machine) noexcept(false) {
  if (!machine.transition_) {
	return {std::make_shared<NotInTransitionError>()};
  }
  machine.transition_();
  WLockGuard guard(machine.state_mu_);
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