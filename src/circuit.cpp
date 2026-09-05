#include "circuit.h"
#include <queue>
#include <iostream>
#include <stdexcept>

std::shared_ptr<Wire> Circuit::get_or_create_wire(const std::string& name) {
    auto it = wires.find(name);
    if (it != wires.end()) {
        return it->second;
    }
    auto w = std::make_shared<Wire>();
    w->name = name;
    w->value = LogicVal::X;
    wires[name] = w;
    return w;
}

void Circuit::add_gate(std::shared_ptr<Gate> gate) {
    gates.push_back(gate);
}

void Circuit::mark_input(const std::string& name) {
    get_or_create_wire(name);
    for (const auto& existing : input_names) {
        if (existing == name) return;
    }
    input_names.push_back(name);
}

void Circuit::mark_output(const std::string& name) {
    get_or_create_wire(name);
    for (const auto& existing : output_names) {
        if (existing == name) return;
    }
    output_names.push_back(name);
}

bool Circuit::topological_sort(std::vector<std::shared_ptr<Gate>>& sorted_gates) const {
    sorted_gates.clear();

    // 1. Map each wire to its driver gate
    std::unordered_map<std::shared_ptr<Wire>, std::shared_ptr<Gate>> wire_driver;
    for (const auto& g : gates) {
        for (const auto& w : g->outputs) {
            if (wire_driver.count(w) > 0) {
                std::cerr << "Error: Multiple drivers detected for wire '" << w->name << "'." << std::endl;
                std::cerr << "Driver 1: Gate '" << wire_driver[w]->name << "'" << std::endl;
                std::cerr << "Driver 2: Gate '" << g->name << "'" << std::endl;
                return false;
            }
            wire_driver[w] = g;
        }
    }

    // 2. Build dependency adjacency list and compute in-degree
    std::unordered_map<std::shared_ptr<Gate>, std::vector<std::shared_ptr<Gate>>> adj;
    std::unordered_map<std::shared_ptr<Gate>, int> in_degree;

    for (const auto& g : gates) {
        in_degree[g] = 0;
    }

    for (const auto& g : gates) {
        for (const auto& w : g->inputs) {
            auto it = wire_driver.find(w);
            if (it != wire_driver.end()) {
                auto driver_gate = it->second;
                adj[driver_gate].push_back(g);
                in_degree[g]++;
            }
        }
    }

    // 3. Initialize queue with gates of in-degree 0
    std::queue<std::shared_ptr<Gate>> q;
    for (const auto& g : gates) {
        if (in_degree[g] == 0) {
            q.push(g);
        }
    }

    // 4. Kahn's topological sort
    while (!q.empty()) {
        auto curr = q.front();
        q.pop();
        sorted_gates.push_back(curr);

        auto it = adj.find(curr);
        if (it != adj.end()) {
            for (const auto& dependent_gate : it->second) {
                in_degree[dependent_gate]--;
                if (in_degree[dependent_gate] == 0) {
                    q.push(dependent_gate);
                }
            }
        }
    }

    // 5. Detect cycles
    if (sorted_gates.size() < gates.size()) {
        std::cerr << "Error: Cycle detected in circuit! The following gates are part of a feedback loop: " << std::endl;
        for (const auto& g : gates) {
            if (in_degree[g] > 0) {
                std::cerr << "  - Gate '" << g->name << "' (Type: " << Gate::to_string(g->type) << ")" << std::endl;
            }
        }
        return false;
    }

    return true;
}

void Circuit::print_summary() const {
    std::cout << "Circuit Summary:" << std::endl;
    std::cout << "  - Primary Inputs (" << input_names.size() << "): ";
    for (size_t i = 0; i < input_names.size(); ++i) {
        std::cout << input_names[i] << (i + 1 < input_names.size() ? ", " : "");
    }
    std::cout << std::endl;

    std::cout << "  - Primary Outputs (" << output_names.size() << "): ";
    for (size_t i = 0; i < output_names.size(); ++i) {
        std::cout << output_names[i] << (i + 1 < output_names.size() ? ", " : "");
    }
    std::cout << std::endl;

    std::cout << "  - Total Gates: " << gates.size() << std::endl;
    std::cout << "  - Total Wires: " << wires.size() << std::endl;
}

void Circuit::reset_wire_values() {
    for (auto& pair : wires) {
        pair.second->value = LogicVal::X;
    }
}
