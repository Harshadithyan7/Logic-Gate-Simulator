#ifndef GATE_H
#define GATE_H

#include "wire.h"
#include <string>
#include <vector>
#include <memory>

enum class GateType {
    AND,
    OR,
    NOT,
    XOR,
    NAND,
    NOR,
    XNOR
};

class Gate {
public:
    std::string name;
    GateType type;
    std::vector<std::shared_ptr<Wire>> inputs;
    std::vector<std::shared_ptr<Wire>> outputs;

    Gate(const std::string& name, GateType type);

    void evaluate();

    static std::string to_string(GateType type);
    static GateType from_string(const std::string& str);
};

#endif // GATE_H
