//
// Created by fanghr on 2020/5/3.
//

#ifndef FSM_EXAMPLES_ELEVATOR_MOTOR_H_
#define FSM_EXAMPLES_ELEVATOR_MOTOR_H_

#include <cstdlib>
#include <fsm/fsm.h>

class Motor : public fsm::FSM<Motor> {
 public:
  struct Up : fsm::Event {};
  struct Down : fsm::Event {};
  struct Stop : fsm::Event {};

 public:
  void React(const fsm::Event &) {}

  void React(const Up &);
  void React(const Down &);
  void React(const Stop &);

  virtual void Entry() = 0;
  void Exit() {}

  enum class Direction {
    kDown = -1,
    kStopped = 0,
    kUp = 1
  };
 protected:
  static Direction direction_;
 public:
  static Direction GetDirection() { return direction_; }
};

#endif //FSM_EXAMPLES_ELEVATOR_MOTOR_H_
