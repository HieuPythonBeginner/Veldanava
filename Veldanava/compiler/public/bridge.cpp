#include "bridge.h"
#include <cstdint>

namespace veldora {

BytecodeBuilder::BytecodeBuilder() = default;

int BytecodeBuilder::add_string(const std::string& s) {
    auto it = string_indices_.find(s);
    if (it != string_indices_.end()) return it->second + 1000;
    int idx = static_cast<int>(strings_.size());
    strings_.push_back(s);
    string_indices_[s] = idx;
    return idx + 1000;
}

int BytecodeBuilder::emit(vm::Opcode op, int a1, int a2, int a3) {
    int pos = static_cast<int>(bytecode_.size());
    bytecode_.emplace_back(op, a1, a2, a3);
    return pos;
}

int BytecodeBuilder::emit_label() {
    int label = next_label_++;
    label_positions_.push_back(static_cast<int>(bytecode_.size()));
    return label;
}

void BytecodeBuilder::patch_jump(int label, int target) {
    if (label >= 0 && label < (int)label_positions_.size()) {
        int pos = label_positions_[label];
        if (pos < (int)bytecode_.size()) {
            bytecode_[pos].arg2 = target;
        }
    }
}

void BytecodeBuilder::patch_call(int label, int argc) {
    if (label >= 0 && label < (int)label_positions_.size()) {
        int pos = label_positions_[label];
        if (pos < (int)bytecode_.size()) {
            bytecode_[pos].arg3 = argc;
        }
    }
}

void BytecodeBuilder::patch_instruction(int pos, vm::Opcode op, int a1, int a2, int a3) {
    if (pos >= 0 && pos < (int)bytecode_.size()) {
        bytecode_[pos] = vm::Instruction(op, a1, a2, a3);
    }
}

void BytecodeBuilder::clear() {
    bytecode_.clear();
    strings_.clear();
    string_indices_.clear();
    label_positions_.clear();
    next_label_ = 0;
}

}
