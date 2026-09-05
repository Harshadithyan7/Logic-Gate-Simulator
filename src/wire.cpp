#include "wire.h"

std::string Wire::to_string(LogicVal val) {
    switch (val) {
        case LogicVal::LOW:  return "0";
        case LogicVal::HIGH: return "1";
        case LogicVal::X:    return "X";
    }
    return "X";
}

LogicVal Wire::from_string(const std::string& str) {
    if (str == "0" || str == "low" || str == "LOW") return LogicVal::LOW;
    if (str == "1" || str == "high" || str == "HIGH") return LogicVal::HIGH;
    return LogicVal::X;
}

LogicVal Wire::from_char(char c) {
    if (c == '0') return LogicVal::LOW;
    if (c == '1') return LogicVal::HIGH;
    return LogicVal::X;
}

std::ostream& operator<<(std::ostream& os, LogicVal val) {
    os << Wire::to_string(val);
    return os;
}
