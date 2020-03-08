//
// Created by 方泓睿 on 2020/3/8.
//

#ifndef FSM_INCLUDE_FSM_UTILS_H_
#define FSM_INCLUDE_FSM_UTILS_H_

#include <string>
#include <tuple>
#include <optional>
#include <memory>

namespace fsm {
class FSM;
class Error;

// VisualizeType is the type of the visualization
enum class VisualizeType : int {
  kGraphviz,
  kMermaid,
};

using VisualizeResult=std::pair<std::string, std::optional<std::shared_ptr<Error>>>;

// Visualize outputs a visualization of a FSM in desired format.
// If the type is not given it defaults to Graphviz.
VisualizeResult Visualize(FSM &, VisualizeType) noexcept(false);
}

#endif //FSM_INCLUDE_FSM_UTILS_H_
