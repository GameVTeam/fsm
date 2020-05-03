//
// Created by fanghr on 2020/5/3.
//

#include <fmt/format.h>

#include "elevator.h"
#include "fsm_list.h"

class Idle;

static void CallMaintenance() {
  fmt::print("> *** calling maintenance ***\n");
}

static void CallFirefighters() {
  fmt::format("> *** calling firefighter ***\n");
}

class Panic : public Elevator {
  void Entry() override {
    SendEvent(Motor::Stop{});
  }
};

class Moving : public Elevator {
  void React(const FloorSensor &e) override {
    size_t floor_expected = current_floor_ + static_cast<int>(Motor::GetDirection());
    if (floor_expected != e.floor) {
      fmt::print("> Floor sensor defect (expected {}, got {}) ", floor_expected, e.floor);
      Transit<Panic>(CallMaintenance);
    } else {
      fmt::print("> Reached floor {}\n", e.floor);
      current_floor_ = e.floor;
      if (e.floor == dest_floor_)
        Transit<Idle>();
    }
  }
};

class Idle : public Elevator {
  void Entry() override {
    SendEvent(Motor::Stop{});
  }

  void React(const Call &e) override {
    dest_floor_ = e.floor;

    if (dest_floor_ == current_floor_)
      return;

    auto action = [] {
      if (dest_floor_ > current_floor_)
        SendEvent(Motor::Up{});
      else if (dest_floor_ < current_floor_)
        SendEvent(Motor::Down{});
    };

    Transit<Moving>(action);
  }
};

void Elevator::React(const Call &) {
  fmt::print("> Call event ignored\n");
}

void Elevator::React(const FloorSensor &) {
  fmt::print("> FloorSensor event ignored\n");
}

void Elevator::React(const Alarm &) {
  Transit<Panic>(CallFirefighters);
}

size_t Elevator::current_floor_ = Elevator::initial_floor_;
size_t Elevator::dest_floor_ = Elevator::initial_floor_;

FSM_INITIAL_STATE(Elevator, Idle);