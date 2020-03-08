//
// Created by 方泓睿 on 2020/3/8.
//

#include <iostream>

#include <fsm/fsm.h>

int main() noexcept(false) {
  auto machine = fsm::FSM("start",
						  {
							  {"first", {"start"}, "one"},
							  {"second", {"start"}, "two"},
							  {"reset", {"one"}, "reset_one"},
							  {"reset", {"two"}, "reset_two"},
							  {"reset", {"reset_one", "reset_two"}, "start"},
						  }, {});

  std::cout << "[graphviz]" << std::endl;
  std::cout << machine.Visualize().first << std::endl;
  std::cout << std::endl;

  std::cout << "[mermaid]" << std::endl;
  std::cout << machine.Visualize(fsm::VisualizeType::kMermaid).first << std::endl;
  std::cout << std::endl;

  return EXIT_SUCCESS;
}