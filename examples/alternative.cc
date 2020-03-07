//
// Created by 方泓睿 on 2020/3/7.
//

#include <fsm/fsm.h>

#include <iostream>

int main() noexcept(false) {
  auto machine = fsm::FSM(
	  "idle",
	  {
		  fsm::EventDesc{
			  .name_="scan",
			  .src_={"idle"},
			  .dst_="scanning",
		  },
		  fsm::EventDesc{
			  .name_="working",
			  .src_={"scanning"},
			  .dst_="scanning",
		  },
		  fsm::EventDesc{
			  .name_="situation",
			  .src_={"scanning"},
			  .dst_="scanning",
		  },
		  fsm::EventDesc{
			  .name_="situation",
			  .src_={"idle"},
			  .dst_="idle",
		  },
		  fsm::EventDesc{
			  .name_="finish",
			  .src_={"scanning"},
			  .dst_="idle",
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