//
// Created by fanghr on 2020/5/3.
//

#include <iostream>

#include <fsm/fsm.h>
#include <fmt/format.h>

struct Toggle : fsm::Event {};

struct Switch : fsm::MealyMachine<Switch> {
  virtual void React(const Toggle &) = 0;

  static void OpenCircuit() {
    fmt::print("* Opening circuit (light goes OFF)\n");
  }

  static void CloseCircuit() {
    fmt::print("* Opening circuit (light goes ON)\n");
  }
};

struct Off;

struct On : Switch {
  void React(const Toggle &) override { Transit<Off>(OpenCircuit); }
};

struct Off : Switch {
  void React(const Toggle &) override { Transit<On>(CloseCircuit); }
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