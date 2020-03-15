//
// Created by 方泓睿 on 2020/3/7.
//

#include <iostream>

#include <fsm/fsm.h>

int main() noexcept(false) {
  fsm::FSM machine(
	  "closed",
	  {
		  fsm::EventDesc{
			  "open",
			  {"closed"},
			  "open",
		  },
		  fsm::EventDesc{
			  "close",
			  {"open"},
			  "closed",
		  }
	  }, {});

  std::cout << machine.Current() << std::endl;

  auto err = machine.FireEvent("open");

  if (err) {
	std::cout << err->What() << std::endl;
	return EXIT_FAILURE;
  }

  std::cout << machine.Current() << std::endl;

  err = machine.FireEvent("close");

  if (err) {
	std::cout << err.value()->What() << std::endl;
	return EXIT_FAILURE;
  }

  std::cout << machine.Current() << std::endl;

  return EXIT_SUCCESS;
}