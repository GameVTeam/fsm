//
// Created by 方泓睿 on 2020/3/7.
//

#include <gtest/gtest.h>

#define TESTING

#include <fsm/fsm.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

class FSMTestGroup : public testing::Test {};

TEST_F(FSMTestGroup, TestSameState) {
  auto machine = fsm::FSM(
	  "start",
	  {
		  {"run", {"start"}, "start"}
	  }, {});
  machine.FireEvent("run");
  if (machine.Current() != "start")
	FAIL() << "expected state to be \"start\"";
}

#pragma clang diagnostic pop