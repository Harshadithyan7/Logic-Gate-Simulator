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

std::string json_escape(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

std::unordered_map<std::string, std::string> parse_json_map(const std::string& input) {
    std::unordered_map<std::string, std::string> map;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '"') {
            size_t key_start = i + 1;
            size_t key_end = input.find('"', key_start);
            if (key_end == std::string::npos) break;
            std::string key = input.substr(key_start, key_end - key_start);
            i = key_end + 1;
            while (i < input.size() && (input[i] == ' ' || input[i] == ':' || input[i] == '\t' || input[i] == '\n')) i++;
            std::string val = "";
            if (i < input.size() && input[i] == '"') {
                size_t val_start = i + 1;
                size_t val_end = input.find('"', val_start);
                if (val_end != std::string::npos) {
                    val = input.substr(val_start, val_end - val_start);
                    i = val_end + 1;
                }
            } else if (i < input.size()) {
                size_t val_start = i;
                while (i < input.size() && input[i] != ',' && input[i] != '}' && input[i] != ' ' && input[i] != '\n') i++;
                val = input.substr(val_start, i - val_start);
            }
            if (!key.empty()) {
                map[key] = val;
            }
        } else {
            i++;
        }
    }
    return map;
}

std::string get_netlist_source(const std::string& arg) {
    std::ifstream file(arg);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    return arg;
}

void handle_json_parse(const std::string& netlist_arg) {
    std::string source = get_netlist_source(netlist_arg);
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto circuit = parser.parse();
        
        std::vector<std::shared_ptr<Gate>> sorted_gates;
        if (!circuit->topological_sort(sorted_gates)) {
            std::cout << "{\"success\": false, \"error\": \"Cycle detected in circuit! Topological sort failed.\"}" << std::endl;
            return;
        }

        std::cout << "{\"success\": true, \"inputs\": [";
        for (size_t i = 0; i < circuit->input_names.size(); ++i) {
            std::cout << "\"" << json_escape(circuit->input_names[i]) << "\"" << (i + 1 < circuit->input_names.size() ? ", " : "");
        }
        std::cout << "], \"outputs\": [";
        for (size_t i = 0; i < circuit->output_names.size(); ++i) {
            std::cout << "\"" << json_escape(circuit->output_names[i]) << "\"" << (i + 1 < circuit->output_names.size() ? ", " : "");
        }
        std::cout << "], \"wires\": [";
        size_t wire_idx = 0;
        for (const auto& kv : circuit->wires) {
            std::cout << "\"" << json_escape(kv.first) << "\"" << (wire_idx + 1 < circuit->wires.size() ? ", " : "");
            wire_idx++;
        }
        std::cout << "], \"gates\": [";
        for (size_t i = 0; i < circuit->gates.size(); ++i) {
            const auto& g = circuit->gates[i];
            std::cout << "{\"name\": \"" << json_escape(g->name) << "\", \"type\": \"" << json_escape(Gate::to_string(g->type)) << "\", \"inputs\": [";
            for (size_t j = 0; j < g->inputs.size(); ++j) {
                std::cout << "\"" << json_escape(g->inputs[j]->name) << "\"" << (j + 1 < g->inputs.size() ? ", " : "");
            }
            std::cout << "], \"outputs\": [";
            for (size_t j = 0; j < g->outputs.size(); ++j) {
                std::cout << "\"" << json_escape(g->outputs[j]->name) << "\"" << (j + 1 < g->outputs.size() ? ", " : "");
            }
            std::cout << "]}" << (i + 1 < circuit->gates.size() ? ", " : "");
        }
        std::cout << "]}" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "{\"success\": false, \"error\": \"" << json_escape(e.what()) << "\"}" << std::endl;
    }
}

void handle_json_simulate(const std::string& netlist_arg, const std::string& inputs_json) {
    std::string source = get_netlist_source(netlist_arg);
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto circuit = parser.parse();
        
        std::vector<std::shared_ptr<Gate>> sorted_gates;
        if (!circuit->topological_sort(sorted_gates)) {
            std::cout << "{\"success\": false, \"error\": \"Cycle detected in circuit! Topological sort failed.\"}" << std::endl;
            return;
        }

        circuit->reset_wire_values();
        auto input_map = parse_json_map(inputs_json);

        for (const auto& input_name : circuit->input_names) {
            if (input_map.count(input_name)) {
                std::string val_str = input_map[input_name];
                if (!val_str.empty()) {
                    char c = std::toupper(val_str[0]);
                    if (c == '0' || c == '1' || c == 'X') {
                        circuit->wires[input_name]->value = Wire::from_char(c);
                    }
                }
            }
        }

        for (const auto& gate : sorted_gates) {
            gate->evaluate();
        }

        std::cout << "{\"success\": true, \"inputs\": {";
        for (size_t i = 0; i < circuit->input_names.size(); ++i) {
            const std::string& in = circuit->input_names[i];
            std::string val = Wire::to_string(circuit->wires[in]->value);
            std::cout << "\"" << json_escape(in) << "\": \"" << val << "\"" << (i + 1 < circuit->input_names.size() ? ", " : "");
        }
        std::cout << "}, \"internal_wires\": {";
        bool first_internal = true;
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
                if (!first_internal) std::cout << ", ";
                first_internal = false;
                std::cout << "\"" << json_escape(name) << "\": \"" << Wire::to_string(pair.second->value) << "\"";
            }
        }
        std::cout << "}, \"outputs\": {";
        for (size_t i = 0; i < circuit->output_names.size(); ++i) {
            const std::string& out = circuit->output_names[i];
            std::string val = Wire::to_string(circuit->wires[out]->value);
            std::cout << "\"" << json_escape(out) << "\": \"" << val << "\"" << (i + 1 < circuit->output_names.size() ? ", " : "");
        }
        std::cout << "}}" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "{\"success\": false, \"error\": \"" << json_escape(e.what()) << "\"}" << std::endl;
    }
}

void handle_json_truthtable(const std::string& netlist_arg) {
    std::string source = get_netlist_source(netlist_arg);
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto circuit = parser.parse();
        
        std::vector<std::shared_ptr<Gate>> sorted_gates;
        if (!circuit->topological_sort(sorted_gates)) {
            std::cout << "{\"success\": false, \"error\": \"Cycle detected in circuit! Topological sort failed.\"}" << std::endl;
            return;
        }

        int num_inputs = circuit->input_names.size();
        if (num_inputs > 16) {
            std::cout << "{\"success\": false, \"error\": \"Truth table generation limit exceeded (max 16 inputs).\"}" << std::endl;
            return;
        }

        std::cout << "{\"success\": true, \"inputs\": [";
        for (size_t i = 0; i < circuit->input_names.size(); ++i) {
            std::cout << "\"" << json_escape(circuit->input_names[i]) << "\"" << (i + 1 < circuit->input_names.size() ? ", " : "");
        }
        std::cout << "], \"outputs\": [";
        for (size_t i = 0; i < circuit->output_names.size(); ++i) {
            std::cout << "\"" << json_escape(circuit->output_names[i]) << "\"" << (i + 1 < circuit->output_names.size() ? ", " : "");
        }
        std::cout << "], \"rows\": [";

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

            std::cout << "{\"inputs\": {";
            for (size_t i = 0; i < circuit->input_names.size(); ++i) {
                const std::string& in = circuit->input_names[i];
                std::cout << "\"" << json_escape(in) << "\": \"" << Wire::to_string(circuit->wires[in]->value) << "\"" << (i + 1 < circuit->input_names.size() ? ", " : "");
            }
            std::cout << "}, \"outputs\": {";
            for (size_t i = 0; i < circuit->output_names.size(); ++i) {
                const std::string& out = circuit->output_names[i];
                std::cout << "\"" << json_escape(out) << "\": \"" << Wire::to_string(circuit->wires[out]->value) << "\"" << (i + 1 < circuit->output_names.size() ? ", " : "");
            }
            std::cout << "}}" << (r + 1 < num_rows ? ", " : "");
        }
        std::cout << "]}" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "{\"success\": false, \"error\": \"" << json_escape(e.what()) << "\"}" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <circuit_file.txt>" << std::endl;
        std::cout << "       " << argv[0] << " --json-parse <netlist>" << std::endl;
        std::cout << "       " << argv[0] << " --json-simulate <netlist> '<inputs_json>'" << std::endl;
        std::cout << "       " << argv[0] << " --json-truthtable <netlist>" << std::endl;
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--json-parse") {
        if (argc < 3) {
            std::cout << "{\"success\": false, \"error\": \"Missing netlist argument for --json-parse\"}" << std::endl;
            return 1;
        }
        handle_json_parse(argv[2]);
        return 0;
    } else if (arg1 == "--json-simulate") {
        if (argc < 3) {
            std::cout << "{\"success\": false, \"error\": \"Missing netlist argument for --json-simulate\"}" << std::endl;
            return 1;
        }
        std::string inputs_json = (argc >= 4) ? argv[3] : "{}";
        handle_json_simulate(argv[2], inputs_json);
        return 0;
    } else if (arg1 == "--json-truthtable") {
        if (argc < 3) {
            std::cout << "{\"success\": false, \"error\": \"Missing netlist argument for --json-truthtable\"}" << std::endl;
            return 1;
        }
        handle_json_truthtable(argv[2]);
        return 0;
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

