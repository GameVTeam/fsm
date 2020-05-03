//
// Created by fanghr on 2020/5/3.
//

#include <iostream>
#include <cstdlib>

#include <fsm/fsm.h>
#include <fmt/format.h>

struct Off;

struct Toggle : fsm::Event {};

struct Switch : fsm::FSM<Switch> {
  static void Reset();

  size_t counter_;

  Switch() : counter_(0) {
    fmt::print("\n"
               "* Switch()\n"
               "  this = {}\n", (void *) this);
  }

  ~Switch() {
    fmt::print("\n"
               "* ~Switch()\n"
               "  this = {}\n", (void *) this);
  }

  Switch &operator=(const Switch &other) {
    fmt::print("\n"
               "* operator=()\n"
               "  this  = {}\n"
               "  other = {}\n", (void *) this, (void *) &other);
    counter_ = other.counter_;
    return *this;
  }

  virtual void React(const Toggle &) {}
  void Entry();
  void Exit();
};

struct On : Switch {
  void React(const Toggle &) override { Transit<Off>(); }
};

struct Off : Switch {
  void React(const Toggle &) override { Transit<On>(); }
};

FSM_INITIAL_STATE(Switch, Off)

void Switch::Reset() {
  fsm::StateList<On, Off>::Reset();
}

void Switch::Entry() {
  counter_++;

  if (IsInState<On>()) fmt::print("\n* On::Entry()\n");
  else if (IsInState<Off>()) fmt::print("\n* Off::Entry()\n");
  else static_assert(true, "");

  assert(current_state_ptr_ == this);
  fmt::print("\n"
             "* Entry\n"
             "  this (cur) = {}\n"
             "  State<On>  = {}\n"
             "  State<Off> = {}\n",
             (void *) this,
             (void *) &State<On>(),
             (void *) &State<Off>());
}

void Switch::Exit() {
  assert(current_state_ptr_ == this);
  fmt::print("\n"
             "* Exit()\n"
             "  this (cur) = {}\n"
             "  State<On>  = {}\n"
             "  State<Off> = {}\n",
             (void *) this,
             (void *) &State<On>(),
             (void *) &State<Off>());
}

int main() {
  Switch::Start();

  while (true) {
    char c{};
    fmt::print("\n"
               "* main()\n"
               "  cur_counter = {}\n"
               "  on_counter  = {}\n"
               "  off_counter = {}\n",
               Switch::current_state_ptr_->counter_,
               Switch::State<On>().counter_,
               Switch::State<Off>().counter_);
    fmt::print("\nt = Toggle, r = Reset, q = Quit ? ");
    std::cin >> c;
    switch (c) {
      case 't': {
        Switch::Dispatch(Toggle{});
        break;
      }
      case 'r': {
        Switch::Reset();
        Switch::Start();
        break;
      }
      case 'q':exit(EXIT_SUCCESS);
      default:fmt::print("> Invalid input\n");
    }
  }
}