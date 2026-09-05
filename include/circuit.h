#ifndef CIRCUIT_H
#define CIRCUIT_H

#include "wire.h"
#include "gate.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

class Circuit {
public:
    std::unordered_map<std::string, std::shared_ptr<Wire>> wires;
    std::vector<std::shared_ptr<Gate>> gates;
    
    // Primary inputs and outputs in order of declaration
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;

    std::shared_ptr<Wire> get_or_create_wire(const std::string& name);
    void add_gate(std::shared_ptr<Gate> gate);
    void mark_input(const std::string& name);
    void mark_output(const std::string& name);

    // Topological sorting of gates. Returns false if a cycle is detected.
    bool topological_sort(std::vector<std::shared_ptr<Gate>>& sorted_gates) const;

    void print_summary() const;
    void reset_wire_values();
};

#endif // CIRCUIT_H
