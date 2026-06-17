#pragma once
#include "instruction.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>

namespace vm {

class VirtualMachine {
public:
    using NativeFunc = int(*)(VirtualMachine* vm, int argc, const int* argv);

    VirtualMachine();

    void load(const std::vector<Instruction>& bytecode);
    void set_strings(const std::vector<std::string>& pool);
    void add_string(const std::string& str);
    void run(int max_instructions = 10000);

    void register_function(const std::string& name, NativeFunc func, int argc);
    NativeFunc get_function(const std::string& name, int& argc) const;

    void register_class(const std::string& name, int field_count);
    void set_class_field(const std::string& class_name, const std::string& field_name, int value);
    int get_class_field(const std::string& class_name, const std::string& field_name) const;

    void register_sanction(const std::string& type, const std::string& name);
    bool is_sanctioned(const std::string& type, const std::string& name) const;

    void print_registers() const;

    int get_register(int idx) const { return registers_[idx]; }
    void set_register(int idx, int val) { registers_[idx] = val; }
    const std::vector<std::string>& get_string_pool() const { return string_pool_; }

private:
    std::vector<Instruction> memory_;
    std::vector<std::string> string_pool_;
    std::unordered_map<std::string, int> variables_;
    int registers_[16];
    int pc_;
    int sp_;
    int cmp_result_;
    std::unordered_map<std::string, std::pair<NativeFunc, int>> function_table_;
    std::unordered_map<int, std::string> function_names_;
    int next_function_index_;
    std::unordered_map<std::string, int> class_table_;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> class_fields_;
    std::unordered_map<std::string, std::unordered_set<std::string>> sanctioned_operators_;
    std::unordered_map<std::string, std::unordered_set<std::string>> sanctioned_funcs_;
};

}