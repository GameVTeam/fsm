//
// Created by 31838 on 3/14/2020.
//

#include <iostream>

#include <fsm/fsm.h>

int main() noexcept(false) {
  fsm::FSM machine(
	  "start",
	  {{"run", {"start"}, "end"}},
	  {{"leave_start", [&](fsm::Event &event) { event.Async(); }}});

  auto err = machine.FireEvent("run");

  if (err)std::cout << err->What() << std::endl;

  err = machine.Transition();

  if (err)std::cout << err->What() << std::endl;

  return EXIT_SUCCESS;
}