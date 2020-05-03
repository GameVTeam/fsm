//
// Created by fanghr on 2020/5/3.
//

#include <iostream>

#include <fsm/fsm.h>
#include <fmt/format.h>

struct Toggle : fsm::Event {};

struct Switch : fsm::MooreMachine<Switch> {
  virtual void React(const Toggle &) = 0;
};

struct Off;

struct On : Switch {
  void Entry() override { fmt::print("* Opening circuit (light goes ON)\n"); }
  void React(const Toggle &) override { Transit<Off>(); }
};

struct Off : Switch {
  void Entry() override { fmt::print("* Opening circuit (light goes OFF)\n"); }
  void React(const Toggle &) override { Transit<On>(); }
};

FSM_INITIAL_STATE(Switch, Off)

int main() {
  Switch::Start();

  fmt::print("> You're facing a light switch\n");

  while (true) {
    char c{};
    fmt::print("\nt = Toggle, q = Quit ? ");
    std::cin >> c;
    switch (c) {
      case 't': {
        fmt::print("> Toggling switch...\n");
        Switch::Dispatch(Toggle{});
        break;
      }
      case 'q':exit(EXIT_SUCCESS);
      default:fmt::print("> Invalid input\n");
    }
  }
}