//
// Created by fanghr on 2020/5/2.
//

#include <fsm/fsm.h>
#include <fmt/format.h>

#include <iostream>

struct Off;

struct Toggle : fsm::Event {};

struct Switch : fsm::FSM<Switch> {
  virtual void React(const Toggle &) {};

  virtual void Entry() {}
  void Exit() {}
};

struct On : Switch {
  void Entry() override { fmt::print("* Switch is ON\n"); }
  void React(const Toggle &) override { Transit<Off>(); }
};

struct Off : Switch {
  void Entry() override { fmt::print("* Switch is OFF\n"); }
  void React(const Toggle &) override { Transit<On>(); }
};

FSM_INITIAL_STATE(Switch, Off);

using FSMHandle = Switch;

int main() {
  Toggle toggle{};

  FSMHandle::Start();

  while (true) {
    char c{};
    fmt::print("\nt = Toggle, q = Quit ? ");
    std::cin >> c;
    switch (c) {
      case 't':fmt::print("> Toggling switch...\n");
        FSMHandle::Dispatch(toggle);
        break;
      case 'q':exit(EXIT_SUCCESS);
      default:fmt::print("> Invalid input\n");
    }
  }
}
