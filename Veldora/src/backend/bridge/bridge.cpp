#include "bridge.h"
#include <stdexcept>
#include <cmath>

namespace veldora {

BridgeCompiler::BridgeCompiler()
    : next_reg_(1) {}

int BridgeCompiler::alloc_reg() {
    int r = next_reg_;
    next_reg_++;
    if (next_reg_ > 15) next_reg_ = 1;
    return r;
}

int BridgeCompiler::get_var_reg(const std::string& name) {
    auto it = var_regs_.find(name);
    if (it != var_regs_.end()) return it->second;
    int r = alloc_reg();
    var_regs_[name] = r;
    return r;
}

int BridgeCompiler::compile_expr(ExprNode* node, veldora::BytecodeBuilder& bb) {
    switch (node->kind) {
        case NodeKind::NumberExpr: {
            auto* n = static_cast<NumberExprNode*>(node);
            int r = alloc_reg();
            bb.emit(vm::Opcode::IMM, r, n->value);
            return r;
        }
        case NodeKind::StringExpr: {
            auto* s = static_cast<StringExprNode*>(node);
            int r = alloc_reg();
            int str_idx = bb.add_string(s->value);
            bb.emit(vm::Opcode::IMM, r, str_idx);
            return r;
        }
        case NodeKind::IdentExpr: {
            auto* id = static_cast<IdentExprNode*>(node);
            int r = alloc_reg();
            int var_reg = get_var_reg(id->name);
            bb.emit(vm::Opcode::MOV, r, var_reg);
            return r;
        }
        case NodeKind::BinaryExpr: {
            auto* b = static_cast<BinaryExprNode*>(node);
            if (b->op == "add" || b->op == "subtract" ||
                b->op == "multiply" || b->op == "divide") {
                int left_reg = compile_expr(b->left.get(), bb);
                int right_reg = compile_expr(b->right.get(), bb);
                int result_reg = left_reg;
                if (b->op == "add") {
                    bb.emit(vm::Opcode::ADD, result_reg, left_reg, right_reg);
                } else if (b->op == "subtract") {
                    bb.emit(vm::Opcode::SUB, result_reg, left_reg, right_reg);
                } else if (b->op == "multiply") {
                    bb.emit(vm::Opcode::MUL, result_reg, left_reg, right_reg);
                } else if (b->op == "divide") {
                    bb.emit(vm::Opcode::DIV, result_reg, left_reg, right_reg);
                }
                return result_reg;
            }

            if (b->op == "and" || b->op == "or") {
                int left_reg = compile_expr(b->left.get(), bb);
                int right_reg = compile_expr(b->right.get(), bb);
                int result_reg = alloc_reg();
                bb.emit(vm::Opcode::IMM, result_reg, b->op == "and" ? (left_reg && right_reg) : (left_reg || right_reg));
                return result_reg;
            }

            if (b->op == "equals" || b->op == "not_equals" ||
                b->op == "less_than" || b->op == "greater_than" ||
                b->op == "less_or_equal" || b->op == "greater_or_equal") {
                int left_reg = compile_expr(b->left.get(), bb);
                int right_reg = compile_expr(b->right.get(), bb);
                int result_reg = alloc_reg();

                bb.emit(vm::Opcode::CMP, left_reg, right_reg);

                int true_label = bb.emit_label();
                int end_label = bb.emit_label();

                if (b->op == "equals") bb.emit(vm::Opcode::JEQ, result_reg, true_label);
                else if (b->op == "not_equals") bb.emit(vm::Opcode::JNE, result_reg, true_label);
                else if (b->op == "less_than") bb.emit(vm::Opcode::JLT, result_reg, true_label);
                else if (b->op == "greater_than") bb.emit(vm::Opcode::JGT, result_reg, true_label);
                else if (b->op == "less_or_equal") bb.emit(vm::Opcode::JLE, result_reg, true_label);
                else if (b->op == "greater_or_equal") bb.emit(vm::Opcode::JGE, result_reg, true_label);

                bb.emit(vm::Opcode::IMM, result_reg, 0);
                int jmp_pos = bb.current_size();
                bb.emit(vm::Opcode::JMP, 0, 0);
                bb.patch_jump(true_label, static_cast<int>(bb.bytecode().size()));
                bb.emit(vm::Opcode::IMM, result_reg, 1);
                bb.patch_instruction(jmp_pos, vm::Opcode::JMP, 0, static_cast<int>(bb.bytecode().size()), 0);
                return result_reg;
            }

            throw std::runtime_error("Unknown binary operator: " + b->op);
        }
        case NodeKind::UnaryExpr: {
            auto* u = static_cast<UnaryExprNode*>(node);
            int val_reg = compile_expr(u->operand.get(), bb);
            if (u->op == "subtract") {
                bb.emit(vm::Opcode::IMM, val_reg, 0);
                bb.emit(vm::Opcode::SUB, val_reg, val_reg, val_reg);
            } else if (u->op == "not") {
                bb.emit(vm::Opcode::IMM, val_reg, 1);
                bb.emit(vm::Opcode::SUB, val_reg, val_reg, val_reg);
            }
            return val_reg;
        }
        case NodeKind::CallExpr: {
            auto* c = static_cast<CallExprNode*>(node);
            std::vector<int> arg_regs;
            for (auto& arg : c->args) {
                arg_regs.push_back(compile_expr(arg.get(), bb));
            }
            for (int i = 0; i < (int)arg_regs.size() && i < 15; i++) {
                bb.emit(vm::Opcode::MOV, i + 1, arg_regs[i]);
            }
            int result_reg = alloc_reg();
            int str_idx = bb.add_string(c->callee);
            bb.emit(vm::Opcode::CALL, result_reg, str_idx, (int)arg_regs.size());
            return result_reg;
        }
        default:
            throw std::runtime_error("Unknown expression kind in bridge");
    }
}

void BridgeCompiler::compile_stmt(StmtNode* node, veldora::BytecodeBuilder& bb) {
    switch (node->kind) {
        case NodeKind::LetStmt: {
            auto* s = static_cast<LetStmtNode*>(node);
            int value_reg = compile_expr(s->value.get(), bb);
            int var_reg = get_var_reg(s->name);
            bb.emit(vm::Opcode::MOV, var_reg, value_reg);
            break;
        }
        case NodeKind::SayStmt: {
            auto* s = static_cast<SayStmtNode*>(node);
            if (s->value) {
                if (s->value->kind == NodeKind::StringExpr) {
                    auto* str = static_cast<StringExprNode*>(s->value.get());
                    int r = alloc_reg();
                    int str_idx = bb.add_string(str->value);
                    bb.emit(vm::Opcode::IMM, r, str_idx);
                    bb.emit(vm::Opcode::PRINT, r, 0);
                } else {
                    int val_reg = compile_expr(s->value.get(), bb);
                    bb.emit(vm::Opcode::PRINT, val_reg, 0);
                }
            }
            break;
        }
        case NodeKind::IfStmt: {
            auto* s = static_cast<IfStmtNode*>(node);
            int cond_reg = compile_expr(s->condition.get(), bb);
            bb.emit(vm::Opcode::CMP, cond_reg, 0);
            int jeq_pos = bb.current_size();
            bb.emit(vm::Opcode::JEQ, 0, 0);
            compile_block(s->then_block.get(), bb);
            int jmp_pos = bb.current_size();
            bb.emit(vm::Opcode::JMP, 0, 0);
            int after_then = bb.current_size();
            if (s->else_block) {
                compile_block(s->else_block.get(), bb);
            }
            int after_if = bb.current_size();
            bb.patch_instruction(jeq_pos, vm::Opcode::JEQ, 0, after_if, 0);
            bb.patch_instruction(jmp_pos, vm::Opcode::JMP, 0, after_if, 0);
            break;
        }
        case NodeKind::WhileStmt: {
            auto* s = static_cast<WhileStmtNode*>(node);
            int loop_start = bb.current_size();
            int cond_reg = compile_expr(s->condition.get(), bb);
            bb.emit(vm::Opcode::CMP, cond_reg, 0);
            int jeq_pos = bb.current_size();
            bb.emit(vm::Opcode::JEQ, 0, 0);
            compile_block(s->body.get(), bb);
            bb.emit(vm::Opcode::JMP, 0, loop_start);
            bb.patch_instruction(jeq_pos, vm::Opcode::JEQ, 0, bb.current_size(), 0);
            break;
        }
        case NodeKind::FuncDecl: {
            auto* s = static_cast<FuncDeclNode*>(node);
            bb.add_string(s->name);
            break;
        }
        case NodeKind::ReturnStmt: {
            auto* s = static_cast<ReturnStmtNode*>(node);
            if (s->value) {
                int val_reg = compile_expr(s->value.get(), bb);
                bb.emit(vm::Opcode::MOV, 0, val_reg);
            }
            bb.emit(vm::Opcode::RET);
            break;
        }
        case NodeKind::ExprStmt: {
            auto* s = static_cast<ExprStmtNode*>(node);
            compile_expr(s->expression.get(), bb);
            break;
        }
        default:
            break;
    }
}

void BridgeCompiler::compile_block(BlockNode* block, veldora::BytecodeBuilder& bb) {
    for (auto& stmt : block->statements) {
        compile_stmt(stmt.get(), bb);
    }
}

std::vector<vm::Instruction> BridgeCompiler::compile(ProgramNode* program, std::vector<std::string>& out_strings) {
    veldora::BytecodeBuilder bb;
    for (auto& stmt : program->statements) {
        compile_stmt(stmt.get(), bb);
    }
    out_strings = bb.strings();
    return bb.bytecode();
}

}
