//
// Created by 方泓睿 on 2020/3/7.
//

#include <iostream>

#include <fsm/fsm.h>

int main() noexcept(false) {
  auto machine = fsm::FSM(
	  "closed",
	  {
		  fsm::EventDesc{
			  .name_="open",
			  .src_={"closed"},
			  .dst_="open",
		  },
		  fsm::EventDesc{
			  .name_="close",
			  .src_={"open"},
			  .dst_="closed",
		  }
	  }, {});

  std::cout << machine.Current() << std::endl;

  auto err = machine.FireEvent("open");

  if (err) {
	std::cout << err.value()->What() << std::endl;
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