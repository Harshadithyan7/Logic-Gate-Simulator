#include "parser.h"
#include "circuit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <cmath>
#include <iomanip>
#include <algorithm>

std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void run_interactive_simulation(const std::shared_ptr<Circuit>& circuit, const std::vector<std::shared_ptr<Gate>>& sorted_gates) {
    std::cout << "\n=== Interactive Simulation ===" << std::endl;
    circuit->reset_wire_values();

    for (const auto& input_name : circuit->input_names) {
        auto wire = circuit->wires[input_name];
        while (true) {
            std::cout << "Enter value for input '" << input_name << "' (0, 1, X): ";
            std::string line;
            if (!std::getline(std::cin, line)) {
                return;
            }
            if (line.empty()) continue;
            
            char val_char = std::toupper(line[0]);
            if (val_char == '0' || val_char == '1' || val_char == 'X') {
                wire->value = Wire::from_char(val_char);
                break;
            } else {
                std::cout << "Invalid input. Please enter 0, 1, or X." << std::endl;
            }
        }
    }

    for (const auto& gate : sorted_gates) {
        gate->evaluate();
    }

    std::cout << "\nSimulation Results:" << std::endl;
    std::cout << "Primary Inputs:" << std::endl;
    for (const auto& input_name : circuit->input_names) {
        std::cout << "  " << input_name << " = " << circuit->wires[input_name]->value << std::endl;
    }
    
    std::cout << "Internal Wires:" << std::endl;
    for (const auto& pair : circuit->wires) {
        const std::string& name = pair.first;
        bool is_input = false;
        for (const auto& in : circuit->input_names) {
            if (in == name) { is_input = true; break; }
        }
        bool is_output = false;
        for (const auto& out : circuit->output_names) {
            if (out == name) { is_output = true; break; }
        }
        if (!is_input && !is_output) {
            std::cout << "  " << name << " = " << pair.second->value << std::endl;
        }
    }

    std::cout << "Primary Outputs:" << std::endl;
    for (const auto& output_name : circuit->output_names) {
        std::cout << "  " << output_name << " = " << circuit->wires[output_name]->value << std::endl;
    }
}

void generate_truth_table(const std::shared_ptr<Circuit>& circuit, const std::vector<std::shared_ptr<Gate>>& sorted_gates) {
    std::cout << "\n=== Truth Table ===" << std::endl;
    
    int num_inputs = circuit->input_names.size();
    if (num_inputs == 0) {
        std::cout << "Circuit has no inputs." << std::endl;
        return;
    }
    
    if (num_inputs > 10) {
        std::cout << "Warning: Circuit has " << num_inputs << " inputs." << std::endl;
        std::cout << "Generating a truth table will require " << (1 << num_inputs) << " rows." << std::endl;
        std::cout << "Do you want to proceed? (y/n): ";
        std::string choice;
        if (!std::getline(std::cin, choice) || (choice != "y" && choice != "Y")) {
            return;
        }
    }

    std::vector<int> input_widths;
    for (const auto& in : circuit->input_names) {
        input_widths.push_back(std::max(3, (int)in.length()));
    }
    std::vector<int> output_widths;
    for (const auto& out : circuit->output_names) {
        output_widths.push_back(std::max(3, (int)out.length()));
    }

    auto print_separator = [&]() {
        std::cout << "+";
        for (int w : input_widths) {
            std::cout << std::string(w + 2, '-') << "+";
        }
        std::cout << "|";
        for (int w : output_widths) {
            std::cout << std::string(w + 2, '-') << "+";
        }
        std::cout << std::endl;
    };

    print_separator();

    std::cout << "|";
    for (size_t i = 0; i < circuit->input_names.size(); ++i) {
        std::cout << " " << std::setw(input_widths[i]) << circuit->input_names[i] << " |";
    }
    std::cout << "|";
    for (size_t i = 0; i < circuit->output_names.size(); ++i) {
        std::cout << " " << std::setw(output_widths[i]) << circuit->output_names[i] << " |";
    }
    std::cout << std::endl;

    print_separator();

    int num_rows = 1 << num_inputs;
    for (int r = 0; r < num_rows; ++r) {
        circuit->reset_wire_values();
        
        for (int i = 0; i < num_inputs; ++i) {
            int bit = (r >> (num_inputs - 1 - i)) & 1;
            circuit->wires[circuit->input_names[i]]->value = bit ? LogicVal::HIGH : LogicVal::LOW;
        }

        for (const auto& gate : sorted_gates) {
            gate->evaluate();
        }

        std::cout << "|";
        for (size_t i = 0; i < circuit->input_names.size(); ++i) {
            std::string val = Wire::to_string(circuit->wires[circuit->input_names[i]]->value);
            std::cout << " " << std::setw(input_widths[i]) << val << " |";
        }
        std::cout << "|";
        for (size_t i = 0; i < circuit->output_names.size(); ++i) {
            std::string val = Wire::to_string(circuit->wires[circuit->output_names[i]]->value);
            std::cout << " " << std::setw(output_widths[i]) << val << " |";
        }
        std::cout << std::endl;
    }

    print_separator();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <circuit_file.txt>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::string source;
    try {
        source = read_file(filepath);
    } catch (const std::exception& e) {
        std::cerr << "Error reading file: " << e.what() << std::endl;
        return 1;
    }

    std::shared_ptr<Circuit> circuit;
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        circuit = parser.parse();
    } catch (const std::exception& e) {
        std::cerr << "Error parsing netlist: " << e.what() << std::endl;
        return 1;
    }

    std::vector<std::shared_ptr<Gate>> sorted_gates;
    if (!circuit->topological_sort(sorted_gates)) {
        std::cerr << "Simulation aborted due to invalid circuit topology." << std::endl;
        return 1;
    }

    std::cout << "Circuit parsed and sorted successfully." << std::endl;
    circuit->print_summary();

    while (true) {
        std::cout << "\nMenu Options:" << std::endl;
        std::cout << "  1. Run Interactive Simulation" << std::endl;
        std::cout << "  2. Generate Truth Table" << std::endl;
        std::cout << "  3. Exit" << std::endl;
        std::cout << "Enter option: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            break;
        }

        if (choice == "1") {
            run_interactive_simulation(circuit, sorted_gates);
        } else if (choice == "2") {
            generate_truth_table(circuit, sorted_gates);
        } else if (choice == "3") {
            std::cout << "Exiting." << std::endl;
            break;
        } else {
            std::cout << "Invalid choice. Please enter 1, 2, or 3." << std::endl;
        }
    }

    return 0;
}
