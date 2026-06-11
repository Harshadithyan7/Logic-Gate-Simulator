#ifndef WIRE_H
#define WIRE_H

#include <string>
#include <iostream>

enum class LogicVal {
    LOW = 0,
    HIGH = 1,
    X = -1
};

struct Wire {
    std::string name;
    LogicVal value = LogicVal::X;

    static std::string to_string(LogicVal val);
    static LogicVal from_string(const std::string& str);
    static LogicVal from_char(char c);
};

std::ostream& operator<<(std::ostream& os, LogicVal val);

#endif // WIRE_H
