//
// Created by fanghr on 2020/5/3.
//

#include <iostream>
#include <random>

#include <fsm/fsm.h>
#include <fmt/format.h>

size_t Random() {
  static std::random_device rd{};
  static std::uniform_int_distribution<size_t> dist{};
  return dist(rd);
}

template<size_t id>
class Off;

static void DumpState(size_t id, const char *state, size_t on_counter, size_t defect_level) {
  fmt::print("* Switch[{}] is {} (on_counter = {}, defect_level = {})\n", id, state, on_counter, defect_level);
}

struct Toggle : fsm::Event {};

template<size_t id>
class DefectiveSwitch : public fsm::FSM<DefectiveSwitch<id>> {
 public:
  static constexpr size_t defect_level_ = id * 2;
  static size_t on_counter_;

  void Reset() { on_counter_ = 0; }

  void React(const fsm::Event &) {}

  virtual void React(const Toggle &) {}
  virtual void Entry() {}
  void Exit() {}
};

template<size_t id>
size_t DefectiveSwitch<id>::on_counter_ = 0;

template<size_t id>
class On : public DefectiveSwitch<id> {
  using Base = DefectiveSwitch<id>;

  void Entry() override {
    Base::on_counter_++;
    DumpState(id, "ON", Base::on_counter_, Base::defect_level_);
  }

  void React(const Toggle &) override {
    Base::template Transit<Off<id>>();
  }
};

template<size_t id>
class Off : public DefectiveSwitch<id> {
  using Base = DefectiveSwitch<id>;

  void Entry() override {
    DumpState(id, "OFF", Base::on_counter_, Base::defect_level_);
  }

  void React(const Toggle &) override {
    if ((Random() % (Base::defect_level_ + 1)) == 0)
      Base::template Transit<On<id>>();
    else {
      fmt::print("* Kzzz kzzzzzz\n");
      Base::template Transit<Off<id>>();
    }
  }
};

FSM_INITIAL_STATE(DefectiveSwitch<0>, Off<0>)
FSM_INITIAL_STATE(DefectiveSwitch<1>, Off<1>)
FSM_INITIAL_STATE(DefectiveSwitch<2>, Off<2>)

using FSMHandle = fsm::FSMList<DefectiveSwitch<0>, DefectiveSwitch<1>, DefectiveSwitch<2>>;

template<size_t id>
void ToggleSingle() {
  fmt::print("> Toggling switch {} ...\n", id);
  DefectiveSwitch<id>::Dispatch(Toggle{});
}

int main() {
  FSMHandle::Start();

  while (true) {
    char c{};
    fmt::print("\n0,1,2 = Toggle single, a = Toggle all, r = Restart, q = Quit ? ");
    std::cin >> c;

    switch (c) {
      case 'q':exit(EXIT_SUCCESS);
      case 'r': {
        FSMHandle::Reset();
        FSMHandle::Start();
        break;
      }
      case '0': {
        ToggleSingle<0>();
        break;
      }
      case '1': {
        ToggleSingle<1>();
        break;
      }
      case '2': {
        ToggleSingle<2>();
        break;
      }
      case 'a': {
        fmt::print("> Toggling all switches...\n");
        FSMHandle::Dispatch(Toggle{});
        break;
      }
      default:fmt::print("> Invalid input\n");
    }
  }
}
