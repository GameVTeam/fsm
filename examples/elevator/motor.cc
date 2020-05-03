//
// Created by fanghr on 2020/5/3.
//

#include <fmt/format.h>

#include "motor.h"

class Stopped : public Motor {
  void Entry() override {
    fmt::print("> Motor: stopped\n");
    direction_ = Direction::kStopped;
  }
};

class Up : public Motor {
  void Entry() override {
    fmt::print("> Motor: moving up\n");
    direction_ = Direction::kUp;
  }
};

class Down : public Motor {
  void Entry() override {
    fmt::print("> Motor: moving down\n");
    direction_ = Direction::kDown;
  }
};

void Motor::React(const Motor::Up &) {
  Transit<::Up>();
}

void Motor::React(const Motor::Down &) {
  Transit<::Down>();
}

void Motor::React(const Motor::Stop &) {
  Transit<::Stopped>();
}

Motor::Direction Motor::direction_ = Motor::Direction::kStopped;

FSM_INITIAL_STATE(Motor, Stopped)