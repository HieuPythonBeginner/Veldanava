#pragma once
#include "../../frontend/ast/ast.h"
#include "../../../Veldanava/compiler/public/bridge.h"
#include <unordered_map>
#include <string>

namespace veldora {

class BridgeCompiler {
public:
    BridgeCompiler();
    std::vector<vm::Instruction> compile(ProgramNode* program, std::vector<std::string>& out_strings);

private:
    int next_reg_;
    std::unordered_map<std::string, int> var_regs_;
    int alloc_reg();
    int get_var_reg(const std::string& name);
    int compile_expr(ExprNode* node, veldora::BytecodeBuilder& bb);
    void compile_stmt(StmtNode* node, veldora::BytecodeBuilder& bb);
    void compile_block(BlockNode* block, veldora::BytecodeBuilder& bb);
};

}
