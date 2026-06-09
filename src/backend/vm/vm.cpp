#include "vm.h"
#include <iostream>

namespace vm {

VirtualMachine::VirtualMachine() : pc_(0), sp_(0) {
    for (int i = 0; i < 16; i++) registers_[i] = 0;
}

void VirtualMachine::load(const std::vector<Instruction>& bytecode) {
    memory_ = bytecode;
}

void VirtualMachine::run() {
    while (pc_ < memory_.size()) {
        Instruction& instr = memory_[pc_++];
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
            case Opcode::PRINT:
                std::cout << registers_[instr.arg1] << "\n";
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