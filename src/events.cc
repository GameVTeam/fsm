//
// Created by 方泓睿 on 2020/3/7.
//

#include "fsm/events.h"

namespace fsm {
void Event::Cancel(const std::vector<std::shared_ptr<Error>> &errors) noexcept {
  if (canceled_)
	return;
  canceled_ = true;
  if (!errors.empty())
	error_ = errors[0];
}

void Event::Async() noexcept {
  async_ = true;
}
}