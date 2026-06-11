#include "vm.h"
#include <iostream>

namespace vm {

VirtualMachine::VirtualMachine() : pc_(0), sp_(0), cmp_result_(0) {
    for (int i = 0; i < 16; i++) registers_[i] = 0;
}

void VirtualMachine::set_strings(const std::vector<std::string>& pool) {
    string_pool_ = pool;
}

void VirtualMachine::add_string(const std::string& str) {
    string_pool_.push_back(str);
}

void VirtualMachine::load(const std::vector<Instruction>& bytecode) {
    memory_ = bytecode;
}

void VirtualMachine::run(int max_instructions) {
    int instructions_executed = 0;
    while (pc_ < memory_.size() && instructions_executed < max_instructions) {
        Instruction& instr = memory_[pc_++];
        instructions_executed++;
        switch (instr.opcode) {
            case Opcode::NOP: break;
            case Opcode::MOV:
                registers_[instr.arg1] = registers_[instr.arg2];
                break;
            case Opcode::IMM:
                registers_[instr.arg1] = instr.arg2;
                break;
            case Opcode::ADD:
                registers_[instr.arg1] = registers_[instr.arg2] + registers_[instr.arg3];
                break;
            case Opcode::SUB:
                registers_[instr.arg1] = registers_[instr.arg2] - registers_[instr.arg3];
                break;
            case Opcode::MUL:
                registers_[instr.arg1] = registers_[instr.arg2] * registers_[instr.arg3];
                break;
            case Opcode::CMP:
                if (registers_[instr.arg1] < registers_[instr.arg2]) cmp_result_ = -1;
                else if (registers_[instr.arg1] == registers_[instr.arg2]) cmp_result_ = 0;
                else cmp_result_ = 1;
                break;
            case Opcode::JMP:
                pc_ = instr.arg2;
                break;
            case Opcode::JEQ:
                if (cmp_result_ == 0) pc_ = instr.arg2;
                break;
            case Opcode::JNE:
                if (cmp_result_ != 0) pc_ = instr.arg2;
                break;
            case Opcode::JLT:
                if (cmp_result_ < 0) pc_ = instr.arg2;
                break;
            case Opcode::JGT:
                if (cmp_result_ > 0) pc_ = instr.arg2;
                break;
            case Opcode::JLE:
                if (cmp_result_ <= 0) pc_ = instr.arg2;
                break;
            case Opcode::JGE:
                if (cmp_result_ >= 0) pc_ = instr.arg2;
                break;
            case Opcode::DIV:
                if (instr.arg3 != 0)
                    registers_[instr.arg1] = registers_[instr.arg2] / instr.arg3;
                break;
            case Opcode::MOD:
                if (instr.arg3 != 0)
                    registers_[instr.arg1] = registers_[instr.arg2] % instr.arg3;
                break;
            case Opcode::PRINT:
                if (instr.arg2 == 0) {
                    int val = registers_[instr.arg1];
                    if (val >= 1000) {
                        int str_idx = val - 1000;
                        if (str_idx >= 0 && str_idx < (int)string_pool_.size()) {
                            std::cout << string_pool_[str_idx] << "\n";
                        }
                    } else {
                        std::cout << val << "\n";
                    }
                } else {
                    std::cout << "\n";
                }
                break;
            case Opcode::RET:
                pc_ = memory_.size();
                break;
            case Opcode::HLT:
                pc_ = memory_.size();
                break;
            default:
                break;
        }
    }
}

void VirtualMachine::print_registers() const {
    for (int i = 0; i < 16; i++) {
        std::cout << "R" << i << " = " << registers_[i] << "\n";
    }
}

}
