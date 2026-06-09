#pragma once
#include "ast/ast.h"
#include "vm/instruction.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace codegen {

class CodeGenerator {
public:
    std::vector<vm::Instruction> generate(ast::ProgramNode* program);

private:
    std::vector<vm::Instruction> instructions_;
    int next_reg_;
    std::unordered_map<std::string, int> var_regs_;

    void gen_stmt(ast::Node* node);
    int gen_expr(ast::Node* node);
};

std::vector<vm::Instruction> generate_code(ast::ProgramNode* program);

}