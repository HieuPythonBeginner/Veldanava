#pragma once
#include "vm_instruction.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <stdexcept>

namespace veldora {

class BytecodeBuilder {
public:
    BytecodeBuilder();

    int add_string(const std::string& s);
    int emit(vm::Opcode op, int a1 = 0, int a2 = 0, int a3 = 0);
    int emit_label();
    int get_label_position(int label) const { return label_positions_.at(label); }
    int current_size() const { return static_cast<int>(bytecode_.size()); }
    void patch_jump(int label, int target);
    void patch_call(int label, int argc);
    void patch_instruction(int pos, vm::Opcode op, int a1, int a2, int a3);

    const std::vector<vm::Instruction>& bytecode() const { return bytecode_; }
    const std::vector<std::string>& strings() const { return strings_; }

    void clear();

private:
    std::vector<vm::Instruction> bytecode_;
    std::vector<std::string> strings_;
    std::unordered_map<std::string, int> string_indices_;
    std::vector<int> label_positions_;
    int next_label_ = 0;
};

}
