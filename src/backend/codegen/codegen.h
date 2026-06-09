#pragma once
#include "vm/instruction.h"
#include "ast/ast.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace codegen {

class CodeGenerator {
public:
    std::vector<vm::Instruction> generate(ast::ProgramNode* program);
    const std::vector<std::string>& get_string_pool() const { return string_pool_; }

    int emit(vm::Opcode op, int a1 = 0, int a2 = 0, int a3 = 0);
    int emit_jmp(vm::Opcode op, int target);
    void patch(int pos, int target);
    int current_pos() const;

private:
    std::vector<vm::Instruction> instructions_;
    int next_reg_;
    std::unordered_map<std::string, int> var_regs_;
    std::vector<int> break_patches_;
    std::vector<int> continue_patches_;
    std::vector<std::string> string_pool_;
    std::unordered_map<std::string, int> string_indices_;

    int add_string(const std::string& s);

    void gen_stmt(ast::Node* node);
    int gen_expr(ast::Node* node);
    int emit_condition_jump(ast::Node* condition);
};

std::vector<vm::Instruction> generate_code(ast::ProgramNode* program);

}

