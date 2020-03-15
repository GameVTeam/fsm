//
// Created by 方泓睿 on 2020/3/7.
//

#include <iostream>

#include <fsm/fsm.h>

int main() noexcept(false) {
  fsm::FSM machine(
	  "idle",
	  {
		  fsm::EventDesc{
			  "scan",
			  {"idle"},
			  "scanning",
		  },
		  fsm::EventDesc{
			  "working",
			  {"scanning"},
			  "scanning",
		  },
		  fsm::EventDesc{
			  "situation",
			  {"scanning"},
			  "scanning",
		  },
		  fsm::EventDesc{
			  "situation",
			  {"idle"},
			  "idle",
		  },
		  fsm::EventDesc{
			  "finish",
			  {"scanning"},
			  "idle",
		  },
	  },
	  {
		  {"scan", [](fsm::Event &event) {
			std::cout << "after_scan: " << event.machine_.get().Current() << std::endl;
		  }},
		  {"working", [](fsm::Event &event) {
			std::cout << "working: " << event.machine_.get().Current() << std::endl;
		  }},
		  {"situation", [](fsm::Event &event) {
			std::cout << "situation: " << event.machine_.get().Current() << std::endl;
		  }},
		  {"finish", [](fsm::Event &event) {
			std::cout << "finish: " << event.machine_.get().Current() << std::endl;
		  }}
	  }
  );

  std::cout << machine.Current() << std::endl;

  auto err = machine.FireEvent("scan");

  if (err) {
	std::cout << err.value()->What() << std::endl;
  }

  std::cout << "1: " << machine.Current() << std::endl;

  err = machine.FireEvent("working");

  if (err) {
	std::cout << err.value()->What() << std::endl;
  }

  std::cout << "2: " << machine.Current() << std::endl;

  err = machine.FireEvent("situation");

  if (err) {
	std::cout << err.value()->What() << std::endl;
  }

  std::cout << "3: " << machine.Current() << std::endl;

  err = machine.FireEvent("finish");

  if (err) {
	std::cout << err.value()->What() << std::endl;
  }

  std::cout << "4: " << machine.Current() << std::endl;

  return EXIT_SUCCESS;
}