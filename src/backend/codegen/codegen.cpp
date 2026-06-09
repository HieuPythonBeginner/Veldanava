#include "codegen.h"
#include "vm/instruction.h"
#include <cstdlib>
#include <cmath>

namespace codegen {

int CodeGenerator::add_string(const std::string& s) {
    auto it = string_indices_.find(s);
    if (it != string_indices_.end()) {
        return 1000 + it->second;
    }
    int idx = string_pool_.size();
    string_pool_.push_back(s);
    string_indices_[s] = idx;
    return 1000 + idx;
}

std::vector<vm::Instruction> CodeGenerator::generate(ast::ProgramNode* program) {
    instructions_.clear();
    next_reg_ = 0;
    var_regs_.clear();
    break_patches_.clear();
    continue_patches_.clear();
    string_pool_.clear();
    string_indices_.clear();
    for (auto& stmt : program->statements) {
        gen_stmt(stmt.get());
    }
    instructions_.emplace_back(vm::Opcode::HLT);
    return instructions_;
}

int CodeGenerator::emit(vm::Opcode op, int a1, int a2, int a3) {
    int pos = instructions_.size();
    instructions_.emplace_back(op, a1, a2, a3);
    return pos;
}

int CodeGenerator::emit_jmp(vm::Opcode op, int target) {
    int pos = instructions_.size();
    instructions_.emplace_back(op, 0, target, 0);
    return pos;
}

void CodeGenerator::patch(int pos, int target) {
    if (pos >= 0 && pos < (int)instructions_.size()) {
        instructions_[pos].arg2 = target;
    }
}

int CodeGenerator::current_pos() const {
    return instructions_.size();
}

int CodeGenerator::emit_condition_jump(ast::Node* condition) {
    if (condition->kind == ast::NodeKind::BinaryExpr) {
        auto* bin = static_cast<ast::BinaryExprNode*>(condition);
        int left_reg = gen_expr(bin->left.get());
        int right_reg = gen_expr(bin->right.get());
        int result_reg = next_reg_++;
        switch (bin->op) {
            case ast::BinaryOp::Lt:
                emit(vm::Opcode::CMP, left_reg, right_reg, 0);
                return emit_jmp(vm::Opcode::JGE, 0);
            case ast::BinaryOp::Gt:
                emit(vm::Opcode::CMP, left_reg, right_reg, 0);
                return emit_jmp(vm::Opcode::JLE, 0);
            case ast::BinaryOp::Le:
                emit(vm::Opcode::CMP, left_reg, right_reg, 0);
                return emit_jmp(vm::Opcode::JGT, 0);
            case ast::BinaryOp::Ge:
                emit(vm::Opcode::CMP, left_reg, right_reg, 0);
                return emit_jmp(vm::Opcode::JLT, 0);
            case ast::BinaryOp::Eq:
                emit(vm::Opcode::CMP, left_reg, right_reg, 0);
                return emit_jmp(vm::Opcode::JNE, 0);
            case ast::BinaryOp::Ne:
                emit(vm::Opcode::CMP, left_reg, right_reg, 0);
                return emit_jmp(vm::Opcode::JEQ, 0);
            default:
                emit(vm::Opcode::ADD, result_reg, left_reg, right_reg);
                emit(vm::Opcode::CMP, result_reg, 0, 0);
                return emit_jmp(vm::Opcode::JEQ, 0);
        }
    } else {
        int reg = gen_expr(condition);
        emit(vm::Opcode::CMP, reg, 0, 0);
        return emit_jmp(vm::Opcode::JEQ, 0);
    }
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
            auto it = var_regs_.find(decl->name);
            int reg;
            if (it != var_regs_.end()) {
                reg = it->second;
            } else {
                reg = next_reg_++;
                var_regs_[decl->name] = reg;
            }
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
        case ast::NodeKind::WhileStmt: {
            auto* whilestmt = static_cast<ast::WhileStmtNode*>(node);
            std::vector<int> saved_break = break_patches_;
            std::vector<int> saved_continue = continue_patches_;
            break_patches_.clear();
            continue_patches_.clear();
            int loop_start = current_pos();
            int cond_jump_pos = emit_condition_jump(whilestmt->condition.get());
            int body_start = current_pos();
            gen_stmt(whilestmt->body.get());
            emit_jmp(vm::Opcode::JMP, loop_start);
            int loop_exit = current_pos();
            patch(cond_jump_pos, loop_exit);
            for (int pos : break_patches_) patch(pos, loop_exit);
            for (int pos : continue_patches_) patch(pos, body_start);
            break_patches_ = saved_break;
            continue_patches_ = saved_continue;
            break;
        }
        case ast::NodeKind::ForStmt: {
            auto* forstmt = static_cast<ast::ForStmtNode*>(node);
            std::vector<int> saved_break = break_patches_;
            std::vector<int> saved_continue = continue_patches_;
            break_patches_.clear();
            continue_patches_.clear();
            int iterator_reg = next_reg_++;
            int bound_reg = -1;
            if (forstmt->bound->kind == ast::NodeKind::FuncCall) {
                auto* call = static_cast<ast::FuncCallNode*>(forstmt->bound.get());
                if (call->name == "range") {
                    bound_reg = gen_expr(call->args[0].get());
                }
            }
            emit(vm::Opcode::IMM, iterator_reg, 0, 0);
            var_regs_[forstmt->iterator] = iterator_reg;
            int loop_start = current_pos();
            int cmp_target = (bound_reg >= 0) ? bound_reg : iterator_reg;
            emit(vm::Opcode::CMP, iterator_reg, cmp_target, 0);
            int cond_jump_pos = emit_jmp(vm::Opcode::JGE, 0);
            int body_start = current_pos();
            gen_stmt(forstmt->body.get());
            int one_reg = next_reg_++;
            emit(vm::Opcode::IMM, one_reg, 1, 0);
            emit(vm::Opcode::ADD, iterator_reg, iterator_reg, one_reg);
            emit_jmp(vm::Opcode::JMP, loop_start);
            int loop_exit = current_pos();
            patch(cond_jump_pos, loop_exit);
            for (int pos : break_patches_) patch(pos, loop_exit);
            for (int pos : continue_patches_) patch(pos, body_start);
            break_patches_ = saved_break;
            continue_patches_ = saved_continue;
            break;
        }
        case ast::NodeKind::BreakStmt: {
            break_patches_.push_back(current_pos());
            emit_jmp(vm::Opcode::JMP, 0);
            break;
        }
        case ast::NodeKind::ContinueStmt: {
            continue_patches_.push_back(current_pos());
            emit_jmp(vm::Opcode::JMP, 0);
            break;
        }
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
            var_regs_.clear();
            gen_stmt(func->body.get());
            break;
        }
        case ast::NodeKind::IfStmt: {
            auto* ifstmt = static_cast<ast::IfStmtNode*>(node);
            int cond_jump_pos = emit_condition_jump(ifstmt->condition.get());
            int then_start = current_pos();
            gen_stmt(ifstmt->then_block.get());
            int after_then = current_pos();
            if (ifstmt->else_block) {
                emit_jmp(vm::Opcode::JMP, current_pos() + 2);
                int else_start = current_pos();
                gen_stmt(ifstmt->else_block.get());
                int after_else = current_pos();
                patch(cond_jump_pos, after_else);
                patch(then_start - 1, after_else);
                patch(else_start - 1, after_else);
            } else {
                patch(cond_jump_pos, after_then);
                patch(then_start - 1, after_then);
            }
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
            if (lit->kind == ast::LiteralKind::Int) {
                instructions_.emplace_back(vm::Opcode::IMM, reg, std::stoi(lit->value), 0);
            } else if (lit->kind == ast::LiteralKind::Bool) {
                instructions_.emplace_back(vm::Opcode::IMM, reg, lit->value == "true" ? 1 : 0, 0);
            } else if (lit->kind == ast::LiteralKind::String) {
                int str_idx = add_string(lit->value);
                instructions_.emplace_back(vm::Opcode::IMM, reg, str_idx, 0);
            } else if (lit->kind == ast::LiteralKind::Float) {
                instructions_.emplace_back(vm::Opcode::IMM, reg, (int)std::stof(lit->value), 0);
            }
            return reg;
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            int left_reg = gen_expr(bin->left.get());
            int right_reg = gen_expr(bin->right.get());
            int result_reg = next_reg_++;
            int unused = 0;
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
                case ast::BinaryOp::Div:
                    instructions_.emplace_back(vm::Opcode::DIV, result_reg, left_reg, right_reg);
                    break;
                case ast::BinaryOp::Mod:
                    instructions_.emplace_back(vm::Opcode::MOD, result_reg, left_reg, right_reg);
                    break;
                default:
                    unused = result_reg;
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
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            if (call->name == "range") {
                int arg_reg = gen_expr(call->args[0].get());
                int reg = next_reg_++;
                instructions_.emplace_back(vm::Opcode::MOV, reg, arg_reg, 0);
                return reg;
            }
            int result_reg = 0;
            for (auto& arg : call->args) {
                result_reg = gen_expr(arg.get());
            }
            return result_reg;
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
