#pragma once

namespace vm {

enum class Opcode {
    NOP, MOV, ADD, SUB, MUL, DIV, MOD,
    CMP, JMP, JEQ, JNE, JLT, JGT, JLE, JGE,
    CALL, RET, HLT,
    LOAD, STORE,
    IMM,
    PRINT,
};

struct Instruction {
    Opcode opcode;
    int arg1;
    int arg2;
    int arg3;

    Instruction(Opcode op, int a1 = 0, int a2 = 0, int a3 = 0)
        : opcode(op), arg1(a1), arg2(a2), arg3(a3) {}
};

}