//
// Created by fanghr on 2020/5/3.
//

#include <fsm/fsm.h>
#include <fmt/format.h>

#include <iostream>

class Off;

struct Toggle : fsm::Event {};

class Switch : public fsm::FSM<Switch> {
  friend class fsm::FSM<Switch>;

  void React(const fsm::Event &) {}

  virtual void React(const Toggle &) {};

  virtual void Entry() {}
  void Exit() {}
 public:
  static void Reset();
};

class On : public Switch {
  void Entry() override { fmt::print("* Switch is ON\n"); }
  void React(const Toggle &) override { Transit<Off>(); }

  size_t counter_;
 public:
  On() : counter_(0) {
    std::cout << "** RESET State=ON" << std::endl;
  }// TODO: Using fmt::print won't print anything. WHY?
};

class Off : public Switch {
  void Entry() override { fmt::print("* Switch is OFF\n"); }
  void React(const Toggle &) override { Transit<On>(); }

  size_t counter_;
 public:
  Off() : counter_(0) {
    std::cout << "** RESET State=OFF" << std::endl;
  } // TODO: Using fmt::print won't print anything. WHY?
};

void Switch::Reset() {
  fmt::print("** RESET Switch\n");
  fsm::StateList<Off, On>::Reset();
}

FSM_INITIAL_STATE(Switch, Off);

using FSMHandle = Switch;

int main() {
  FSMHandle::Start();

  while (true) {
    char c{};
    fmt::print("\nt = Toggle, q = Quit, r = Reset ? ");
    std::cin >> c;
    switch (c) {
      case 't': {
        fmt::print("> Toggling switch...\n");
        FSMHandle::Dispatch(Toggle{});
        break;
      }
      case 'r': {
        FSMHandle::Reset();
        FSMHandle::Start();
        break;
      }
      case 'q':exit(EXIT_SUCCESS);
      default:fmt::print("> Invalid input\n");
    }
  }
}
