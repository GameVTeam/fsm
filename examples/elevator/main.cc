//
// Created by fanghr on 2020/5/3.
//

#include <iostream>
#include <fmt/format.h>

#include "fsm_list.h"

int main() {
  FSMList::Start();

  Call call{};
  FloorSensor floor_sensor{};

  while (true) {
    char c{};
    fmt::print("c = Call, f = FloorSensor, a = Alarm, q = Quit ? ");
    std::cin >> c;

    switch (c) {
      case 'c': {
        fmt::print("Floor ? ");
        std::cin >> call.floor;
        SendEvent(call);
        break;
      }
      case 'f': {
        fmt::print("Floor ? ");
        std::cin >> floor_sensor.floor;
        SendEvent(floor_sensor);
        break;
      }
      case 'a': {
        SendEvent(Alarm{});
        break;
      }
      case 'q': {
        fmt::print("> Captain on the bridge!\n");
        exit(EXIT_SUCCESS);
      }
      default:fmt::print("> Invalid input\n");
    }
  }
}