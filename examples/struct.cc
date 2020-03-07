//
// Created by 方泓睿 on 2020/3/7.
//

#include <utility>
#include <iostream>

#include <fsm/fsm.h>

class Door {
 public:
  std::string to_;
  fsm::FSM machine_;

  explicit Door(std::string to) noexcept(false):
	  machine_("closed", {
		  {"open", {"closed"}, "open"},
		  {"close", {"open"}, "closed"}
	  }, {
				   {"enter_state", [&](fsm::Event &event) { EnterState(event); }}
			   }), to_(std::move(to)) {}

  void EnterState(fsm::Event &event) noexcept(false) {
	std::cout << "The door to " << to_ << " is " << event.dst_ << std::endl;
  }
};

int main() noexcept(false) {
  auto door = Door("老 八 撤 🔒");

  auto err = door.machine_.FireEvent("open");
  if (err)
	std::cout << err.value()->What();

  err = door.machine_.FireEvent("close");
  if (err)
	std::cout << err.value()->What();

  return EXIT_SUCCESS;
}