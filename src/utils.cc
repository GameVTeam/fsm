//
// Created by 方泓睿 on 2020/3/8.
//

#include <sstream>
#include <map>

#include "fsm/fsm.h"

namespace fsm {
class Visualizer {
 public:
  virtual std::string Visualize(FSM &) noexcept(false) = 0;
};

class MermaidVisualizer : public Visualizer {
 public:
  std::string Visualize(FSM &machine) noexcept(false) override {
	// we sort the keys alphabetically to have a reproducible graph output
	std::vector<impl::EKey> sorted_e_keys{};

	for (const auto &kv:machine.transitions_)
	  sorted_e_keys.push_back(kv.first);

	std::sort(sorted_e_keys.begin(), sorted_e_keys.end(), [](const impl::EKey &lhs, const impl::EKey &rhs) {
	  return lhs.src_ < rhs.src_;
	});

	std::stringstream buffer{};

	buffer << "graph fsm" << std::endl;

	for (const auto &key:sorted_e_keys) {
	  auto val = machine.transitions_[key];
	  buffer << "    " << key.src_ << " -->|" << key.event_ << "| " << val << std::endl;
	}

	return buffer.str();
  }
};

class GraphvizVisualizer : public Visualizer {
 public:
  std::string Visualize(FSM &machine) noexcept(false) override {
	std::stringstream buffer{};
	std::map<std::string, int> states{};

	// we sort the keys alphabetically to have a reproducible graph output
	std::vector<impl::EKey> sorted_e_keys{};

	for (const auto &kv:machine.transitions_)
	  sorted_e_keys.push_back(kv.first);

	std::sort(sorted_e_keys.begin(), sorted_e_keys.end(), [](const impl::EKey &lhs, const impl::EKey &rhs) {
	  return lhs.src_ < rhs.src_;
	});

	buffer << "digraph fsm {" << std::endl;

	// make sure the initial state is at top
	for (const auto &key:sorted_e_keys) {
	  auto val = machine.transitions_[key];
	  if (key.src_ == machine.current_) {
		states[key.src_]++;
		states[val]++;
		buffer << "    \"" << key.src_ << "\" -> \"" << val << "\" [ label = \"" << key.event_ << "\" ];" << std::endl;
	  }
	}

	for (const auto &key:sorted_e_keys) {
	  auto val = machine.transitions_[key];
	  if (key.src_ != machine.current_) {
		states[key.src_]++;
		states[val]++;
		buffer << "    \"" << key.src_ << "\" -> \"" << val << "\" [ label = \"" << key.event_ << "\" ];" << std::endl;
	  }
	}

	buffer << std::endl;

	std::vector<std::string> sorted_state_keys{};

	for (const auto &kv:states)
	  sorted_state_keys.push_back(kv.first);

	std::sort(sorted_state_keys.begin(), sorted_state_keys.end());

	for (const auto &key:sorted_state_keys)
	  buffer << "    \"" << key << "\";" << std::endl;

	buffer << "}";

	return buffer.str();
  }
};

VisualizeResult Visualize(FSM &machine, VisualizeType visualize_type) noexcept(false) {
  switch (visualize_type) {
	case VisualizeType::kGraphviz:
	  return std::make_pair(GraphvizVisualizer().Visualize(machine),
							std::optional<std::shared_ptr<Error>>{});
	case VisualizeType::kMermaid:
	  return std::make_pair(MermaidVisualizer().Visualize(machine),
							std::optional<std::shared_ptr<Error>>{});
	default: return std::make_pair("", std::make_shared<UnknownVisualizeTypeError>());
  }
}
}