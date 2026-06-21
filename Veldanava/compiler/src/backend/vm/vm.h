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
    const std::unordered_map<std::string, int>& get_function_call_counts() const { return function_call_counts_; }
    void increment_call_count(const std::string& name) { function_call_counts_[name]++; }

    const std::vector<Instruction>& get_memory() const { return memory_; }
    int get_last_instruction_count() const { return last_instruction_count_; }
    void set_last_instruction_count(int v) { last_instruction_count_ = v; }

private:
    std::vector<Instruction> memory_;
    std::vector<std::string> string_pool_;
    std::unordered_map<std::string, int> variables_;
    int registers_[16];
    int pc_;
    int sp_;
    int cmp_result_;
    int next_instance_id_;
    std::unordered_map<std::string, std::pair<NativeFunc, int>> function_table_;
    std::unordered_map<int, std::string> function_names_;
    std::unordered_map<std::string, int> class_table_;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> class_fields_;
    std::unordered_map<std::string, std::vector<std::string>> class_layout_;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> class_field_indices_;
    std::unordered_map<int, std::string> object_classes_;
    std::unordered_map<int, std::unordered_map<int, int>> object_fields_;
    std::unordered_map<std::string, std::unordered_set<std::string>> sanctioned_operators_;
    std::unordered_map<std::string, std::unordered_set<std::string>> sanctioned_funcs_;
    std::unordered_map<int, std::vector<int>> arrays_;
    int next_array_id_;
    std::unordered_map<std::string, int> function_call_counts_;
    int last_instruction_count_ = 0;
};

}
