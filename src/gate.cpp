#include "gate.h"
#include <algorithm>
#include <stdexcept>

Gate::Gate(const std::string& name, GateType type)
    : name(name), type(type) {}

void Gate::evaluate() {
    if (outputs.empty()) return;

    LogicVal result = LogicVal::X;

    switch (type) {
        case GateType::AND: {
            if (inputs.empty()) {
                result = LogicVal::X;
            } else {
                bool has_low = false;
                bool has_x = false;
                for (const auto& w : inputs) {
                    if (w->value == LogicVal::LOW) {
                        has_low = true;
                        break;
                    } else if (w->value == LogicVal::X) {
                        has_x = true;
                    }
                }
                if (has_low) {
                    result = LogicVal::LOW;
                } else if (has_x) {
                    result = LogicVal::X;
                } else {
                    result = LogicVal::HIGH;
                }
            }
            break;
        }
        case GateType::OR: {
            if (inputs.empty()) {
                result = LogicVal::X;
            } else {
                bool has_high = false;
                bool has_x = false;
                for (const auto& w : inputs) {
                    if (w->value == LogicVal::HIGH) {
                        has_high = true;
                        break;
                    } else if (w->value == LogicVal::X) {
                        has_x = true;
                    }
                }
                if (has_high) {
                    result = LogicVal::HIGH;
                } else if (has_x) {
                    result = LogicVal::X;
                } else {
                    result = LogicVal::LOW;
                }
            }
            break;
        }
        case GateType::NOT: {
            if (inputs.empty()) {
                result = LogicVal::X;
            } else {
                if (inputs[0]->value == LogicVal::LOW) {
                    result = LogicVal::HIGH;
                } else if (inputs[0]->value == LogicVal::HIGH) {
                    result = LogicVal::LOW;
                } else {
                    result = LogicVal::X;
                }
            }
            break;
        }
        case GateType::XOR: {
            if (inputs.empty()) {
                result = LogicVal::X;
            } else {
                bool has_x = false;
                int high_count = 0;
                for (const auto& w : inputs) {
                    if (w->value == LogicVal::X) {
                        has_x = true;
                        break;
                    } else if (w->value == LogicVal::HIGH) {
                        high_count++;
                    }
                }
                if (has_x) {
                    result = LogicVal::X;
                } else {
                    result = (high_count % 2 == 1) ? LogicVal::HIGH : LogicVal::LOW;
                }
            }
            break;
        }
        case GateType::NAND: {
            if (inputs.empty()) {
                result = LogicVal::X;
            } else {
                bool has_low = false;
                bool has_x = false;
                for (const auto& w : inputs) {
                    if (w->value == LogicVal::LOW) {
                        has_low = true;
                        break;
                    } else if (w->value == LogicVal::X) {
                        has_x = true;
                    }
                }
                if (has_low) {
                    result = LogicVal::HIGH;
                } else if (has_x) {
                    result = LogicVal::X;
                } else {
                    result = LogicVal::LOW;
                }
            }
            break;
        }
        case GateType::NOR: {
            if (inputs.empty()) {
                result = LogicVal::X;
            } else {
                bool has_high = false;
                bool has_x = false;
                for (const auto& w : inputs) {
                    if (w->value == LogicVal::HIGH) {
                        has_high = true;
                        break;
                    } else if (w->value == LogicVal::X) {
                        has_x = true;
                    }
                }
                if (has_high) {
                    result = LogicVal::LOW;
                } else if (has_x) {
                    result = LogicVal::X;
                } else {
                    result = LogicVal::HIGH;
                }
            }
            break;
        }
        case GateType::XNOR: {
            if (inputs.empty()) {
                result = LogicVal::X;
            } else {
                bool has_x = false;
                int high_count = 0;
                for (const auto& w : inputs) {
                    if (w->value == LogicVal::X) {
                        has_x = true;
                        break;
                    } else if (w->value == LogicVal::HIGH) {
                        high_count++;
                    }
                }
                if (has_x) {
                    result = LogicVal::X;
                } else {
                    result = (high_count % 2 == 1) ? LogicVal::LOW : LogicVal::HIGH;
                }
            }
            break;
        }
    }

    for (auto& w : outputs) {
        w->value = result;
    }
}

std::string Gate::to_string(GateType type) {
    switch (type) {
        case GateType::AND:  return "AND";
        case GateType::OR:   return "OR";
        case GateType::NOT:  return "NOT";
        case GateType::XOR:  return "XOR";
        case GateType::NAND: return "NAND";
        case GateType::NOR:  return "NOR";
        case GateType::XNOR: return "XNOR";
    }
    return "UNKNOWN";
}

GateType Gate::from_string(const std::string& str) {
    if (str == "AND" || str == "and") return GateType::AND;
    if (str == "OR" || str == "or") return GateType::OR;
    if (str == "NOT" || str == "not") return GateType::NOT;
    if (str == "XOR" || str == "xor") return GateType::XOR;
    if (str == "NAND" || str == "nand") return GateType::NAND;
    if (str == "NOR" || str == "nor") return GateType::NOR;
    if (str == "XNOR" || str == "xnor") return GateType::XNOR;
    throw std::runtime_error("Unknown gate type: " + str);
}
