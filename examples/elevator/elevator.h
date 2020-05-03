//
// Created by fanghr on 2020/5/3.
//

#ifndef FSM_EXAMPLES_ELEVATOR_ELEVATOR_H_
#define FSM_EXAMPLES_ELEVATOR_ELEVATOR_H_

#include <cstdlib>

#include <fsm/fsm.h>

struct FloorEvent : fsm::Event {
  size_t floor;
};

struct Call : FloorEvent {};
struct FloorSensor : FloorEvent {};
struct Alarm : fsm::Event {};

class Elevator : public fsm::FSM<Elevator> {
 public:
  void React(const fsm::Event &) {}

  virtual void React(const Call &);
  virtual void React(const FloorSensor &);
  void React(const Alarm &);

  virtual void Entry() {}
  void Exit() {}

 protected:
  static constexpr size_t initial_floor_ = 0;
  static size_t current_floor_;
  static size_t dest_floor_;
};

#endif //FSM_EXAMPLES_ELEVATOR_ELEVATOR_H_
