# fsm

fsm is a zero-dependency C++11 finite state machine library.

## Basic Example 

From examples/simple.cc:
```c++
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
```

For more usages, check examples in the examples folder.

## License

fsm is licensed under MIT license.

---
## [Starfall](https://open.spotify.com/track/2fcgSnUoEvzDVxrQZMx0bu?si=FXrl9FXQRJmpEeVn9qWaNw) is really AWESOME! 