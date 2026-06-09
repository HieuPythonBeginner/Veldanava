#pragma once
#include "instruction.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace vm {

class VirtualMachine {
public:
    VirtualMachine();

    void load(const std::vector<Instruction>& bytecode);
    void run();

    void print_registers() const;

private:
    std::vector<Instruction> memory_;
    std::unordered_map<std::string, int> variables_;
    int registers_[16];
    int pc_;
    int sp_;
};

}