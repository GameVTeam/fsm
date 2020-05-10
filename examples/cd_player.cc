//
// Created by fanghr on 2020/5/10.
//

#include <iostream>

#include <fmt/format.h>
#include <fsm/fsm.h>

class Player : public fsm::FSM<Player> {
  friend class FSM;

  std::string cd_title_;
  bool autoplay_ = false;

 public:
  enum States { kStopped, kOpen, kEmpty, kPlaying, kPaused };

  explicit Player(StateType init_state = kEmpty) : FSM(init_state) {}

  void SetAutoplay(bool f) { autoplay_ = f; }
  bool IsAutoplay() const { return autoplay_; }

  const std::string &GetCDTitle() const { return cd_title_; }

 public: // events
  struct Play {};
  struct OpenClose {};
  struct CDDetected { std::string title; };
  struct Stop {};
  struct Pause {};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"
 private: // guards
  bool IsAutoplay(const CDDetected &) const { return autoplay_; }
  bool IsBadCD(const CDDetected &cd) const { return cd.title.empty() || cd.title[0] != '#'; }

 private: // actions
  void StartPlayback(const Play &) { fmt::print("* ▶️ Starting playback\n"); }

  void StartAutoplay(const CDDetected &cd) {
    cd_title_ = cd.title;
    fmt::print("* ▶️ Starting playback\n");
  }

  void OpenDrawer(const CDDetected &) {
    fmt::print("* ⏏️ Ejecting bad CD\n");
  }
  void OpenDrawer(const OpenClose &) {
    cd_title_.clear();
    fmt::print("* ⏏️ Opening drawer\n");
  }

  void CloseDrawer(const OpenClose &) { fmt::print("* ℹ️ Closing drawer\n"); }

  void StoreCDInfo(const CDDetected &cd) {
    cd_title_ = cd.title;
    fmt::print("* ❗️ Detected CD '{}'\n", cd_title_);
  }

  void StopPlayback(const Stop &) { fmt::print("* ⏹ Stopping playback\n"); }
  void PausePlayback(const Pause &) { fmt::print("* ⏸ Pausing playback\n"); }

  void StopAndOpen(const OpenClose &) {
    cd_title_.clear();
    fmt::print("* ⏹ Stopping playback and ⏏️ opening drawer\n");
  }

  void ResumePlayback(const Play &) { fmt::print("* ▶️ Resuming playback\n"); }
#pragma clang diagnostic pop
 private:
  using TransitionTable = Table<
  MemFnRow<kStopped, Play, kPlaying, &Player::StartPlayback>,
  MemFnRow<kStopped, OpenClose, kOpen, &Player::OpenDrawer>,
  MemFnRow<kOpen, OpenClose, kEmpty, &Player::CloseDrawer>,
  MemFnRow<kEmpty, OpenClose, kOpen, &Player::OpenDrawer>,
  MemFnRow<kEmpty, CDDetected, kOpen, &Player::OpenDrawer, &Player::IsBadCD>,
  MemFnRow<kEmpty, CDDetected, kPlaying, &Player::StartAutoplay, &Player::IsAutoplay>,
  MemFnRow<kEmpty, CDDetected, kStopped, &Player::StoreCDInfo>,
  MemFnRow<kPlaying, Stop, kStopped, &Player::StopPlayback>,
  MemFnRow<kPlaying, Pause, kPaused, &Player::PausePlayback>,
  MemFnRow<kPaused, Play, kPlaying, &Player::ResumePlayback>,
  MemFnRow<kPaused, Stop, kStopped, &Player::StopPlayback>,
  MemFnRow<kPlaying, OpenClose, kOpen, &Player::StopAndOpen>,
  MemFnRow<kPaused, OpenClose, kOpen, &Player::StopAndOpen>>;
};

[[noreturn]] void Panic(const std::string &info) {
  fmt::print("panic: {}\n", info);
  exit(EXIT_FAILURE);
}

int main() {
  Player player{};
  char command{};

  auto display_help_info = [] {
    fmt::print(""
               "CD Player\n\n"
               "Available commands:"
               "💤 q = quit\n"
               "⏏️ t = open/close drawer\n"
               "ℹ️ i = insert CD\n"
               "🔄 a = toggle auto playback;\n"
               "▶️ s = start playback\n"
               "⏹ o = stop playback\n"
               "⏸ p = pause playback\n"
               "❓ h = show this help message\n\n");
  };

  auto display_prompt = [&] {
    fmt::print("autoplay: {:>3} {:*^40} {}> ",
               player.IsAutoplay() ? "on" : "off",
               !player.GetCDTitle().empty() ? player.GetCDTitle() : "<EMPTY>",
               [&]() -> std::string {
                 switch (player.CurrentState()) {
                   case Player::kPaused:return "⏸";
                   case Player::kPlaying:return "▶️";
                   case Player::kOpen:return "⏏️";
                   case Player::kEmpty:return "💤";
                   case Player::kStopped:return "⏹";
                 }
                 Panic("unreachable code");
               }()
    );
  };

  display_help_info();

  while (true) {
    display_prompt();
    if (std::cin >> command) {
      switch (command) {
        case 't': {
          player(Player::OpenClose{});
          break;
        }
        case 'i': {
          fmt::print("The title of inserted CD is (start with '#')❔ ");
          std::string title{};
          std::cin >> title;
          player(Player::CDDetected{title});
          break;
        }
        case 'a': {
          player.SetAutoplay(!player.IsAutoplay());
          break;
        }
        case 'h': {
          display_help_info();
          break;
        }
        case 'q': {
          player(Player::OpenClose{});
          goto exit;
        }
        case 's': {
          player(Player::Play{});
          break;
        }
        case 'o': {
          player(Player::Stop{});
          break;
        }
        case 'p': {
          player(Player::Pause{});
          break;
        }
        default:fmt::print("unknown command {}\n", command);
      }
    } else
      Panic("io error");
  }

  exit:
  return EXIT_SUCCESS;
}