#include "codegen.h"
#include "vm/instruction.h"
#include <cstdlib>

namespace codegen {

std::vector<vm::Instruction> CodeGenerator::generate(ast::ProgramNode* program) {
    instructions_.clear();
    next_reg_ = 0;
    for (auto& stmt : program->statements) {
        gen_stmt(stmt.get());
    }
    instructions_.emplace_back(vm::Opcode::HLT);
    return instructions_;
}

void CodeGenerator::gen_stmt(ast::Node* node) {
    switch (node->kind) {
        case ast::NodeKind::Block: {
            auto* block = static_cast<ast::BlockNode*>(node);
            for (auto& stmt : block->statements) {
                gen_stmt(stmt.get());
            }
            break;
        }
        case ast::NodeKind::VarDecl: {
            auto* decl = static_cast<ast::VarDeclNode*>(node);
            int reg = next_reg_++;
            var_regs_[decl->name] = reg;
            if (decl->initializer) {
                int init_reg = gen_expr(decl->initializer.get());
                instructions_.emplace_back(vm::Opcode::MOV, reg, init_reg, 0);
            }
            break;
        }
        case ast::NodeKind::ReturnStmt: {
            auto* ret = static_cast<ast::ReturnStmtNode*>(node);
            int reg = gen_expr(ret->value.get());
            instructions_.emplace_back(vm::Opcode::MOV, 0, reg, 0);
            instructions_.emplace_back(vm::Opcode::RET);
            break;
        }
        case ast::NodeKind::ForStmt:
            break;
        case ast::NodeKind::BinaryExpr:
            gen_expr(node);
            break;
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            if (call->name == "print" || call->name == "scribe") {
                for (auto& arg : call->args) {
                    int reg = gen_expr(arg.get());
                    instructions_.emplace_back(vm::Opcode::PRINT, reg, 0, 0);
                }
            }
            break;
        }
        case ast::NodeKind::FuncDef: {
            auto* func = static_cast<ast::FuncDefNode*>(node);
            gen_stmt(func->body.get());
            break;
        }
        default:
            break;
    }
}

int CodeGenerator::gen_expr(ast::Node* node) {
    switch (node->kind) {
        case ast::NodeKind::Literal: {
            auto* lit = static_cast<ast::LiteralNode*>(node);
            int reg = next_reg_++;
            instructions_.emplace_back(vm::Opcode::IMM, reg, std::stoi(lit->value), 0);
            return reg;
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            int left_reg = gen_expr(bin->left.get());
            int right_reg = gen_expr(bin->right.get());
            int result_reg = next_reg_++;
            switch (bin->op) {
                case ast::BinaryOp::Add:
                    instructions_.emplace_back(vm::Opcode::ADD, result_reg, left_reg, right_reg);
                    break;
                case ast::BinaryOp::Sub:
                    instructions_.emplace_back(vm::Opcode::SUB, result_reg, left_reg, right_reg);
                    break;
                case ast::BinaryOp::Mul:
                    instructions_.emplace_back(vm::Opcode::MUL, result_reg, left_reg, right_reg);
                    break;
                default:
                    break;
            }
            return result_reg;
        }
        case ast::NodeKind::Identifier: {
            auto* id = static_cast<ast::IdentifierNode*>(node);
            auto it = var_regs_.find(id->name);
            if (it != var_regs_.end()) {
                return it->second;
            }
            int reg = next_reg_++;
            instructions_.emplace_back(vm::Opcode::IMM, reg, 0, 0);
            return reg;
        }
        default:
            return 0;
    }
}

std::vector<vm::Instruction> generate_code(ast::ProgramNode* program) {
    CodeGenerator gen;
    return gen.generate(program);
}

}