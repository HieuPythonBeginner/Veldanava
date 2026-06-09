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
    void set_strings(const std::vector<std::string>& pool);
    void add_string(const std::string& str);
    void run();

    void print_registers() const;

private:
    std::vector<Instruction> memory_;
    std::vector<std::string> string_pool_;
    std::unordered_map<std::string, int> variables_;
    int registers_[16];
    int pc_;
    int sp_;
    int cmp_result_;
};

}