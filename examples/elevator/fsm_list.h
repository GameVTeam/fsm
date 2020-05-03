//
// Created by fanghr on 2020/5/3.
//

#ifndef FSM_EXAMPLES_ELEVATOR_FSM_LIST_H_
#define FSM_EXAMPLES_ELEVATOR_FSM_LIST_H_

#include <fsm/fsm.h>

#include "elevator.h"
#include "motor.h"

using FSMList = fsm::FSMList<Motor, Elevator>;

template<typename E>
void SendEvent(const E &event) {
  FSMList::template Dispatch<E>(event);
}

#endif //FSM_EXAMPLES_ELEVATOR_FSM_LIST_H_
