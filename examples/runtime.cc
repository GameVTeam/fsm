//
// Created by 31838 on 3/20/2020.
//

#include <iostream>

#include <fsm/fsm.h>

int main() noexcept(false) {
  fsm::FSM machine
	  ("start", {{"run", {"start"}, "end"}}, {{"run", [](fsm::Event &) { std::cout << "1: run" << std::endl; }}});

  machine.FireEvent("run");

  machine.SetCallback("run", [](fsm::Event &) {
	std::cout << "2: run" << std::endl;
  });

  machine.SetState("start");

  machine.FireEvent("run");

  return EXIT_SUCCESS;
}