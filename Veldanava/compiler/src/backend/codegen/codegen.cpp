#include "codegen.h"
#include "frontend/module/module_loader.h"
#include "vm/instruction.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace codegen {

static std::string required_module_for_function(const std::string& name);

int CodeGenerator::add_string(const std::string& s) {
    auto it = string_indices_.find(s);
    if (it != string_indices_.end()) {
        return 1000 + it->second;
    }
    int idx = static_cast<int>(string_pool_.size());
    string_pool_.push_back(s);
    string_indices_[s] = idx;
    return 1000 + idx;
}

void CodeGenerator::incorporate_module(const std::string& module) {
    incorporated_modules_.insert(module);
}

bool CodeGenerator::is_module_function_available(const std::string& name) const {
    std::string module = required_module_for_function(name);
    if (module.empty()) return true;
    return incorporated_modules_.find(module) != incorporated_modules_.end();
}

std::string CodeGenerator::method_key(const std::string& class_name, const std::string& method_name) const {
    return class_name + "::" + method_name;
}

static bool starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

static std::string base_type(const std::string& type) {
    std::string current = type;
    while (starts_with(current, "ptr ") || starts_with(current, "ref ")) {
        current = current.substr(4);
    }
    return current;
}

std::vector<vm::Instruction> CodeGenerator::generate(ast::ProgramNode* program) {
    instructions_.clear();
    next_reg_ = 0;
    next_array_id_ = 0;
    var_regs_.clear();
    break_patches_.clear();
    continue_patches_.clear();
    string_pool_.clear();
    string_indices_.clear();
    incorporated_modules_.clear();
    function_table_.clear();
    class_names_.clear();
    class_fields_.clear();
    field_indices_.clear();

    var_scope_stack_.clear();
    var_scope_stack_.push_back(var_regs_);

    for (auto& stmt : program->statements) {
        if (stmt->kind == ast::NodeKind::GenesisDecl) {
            auto* decl = static_cast<ast::GenesisDeclNode*>(stmt.get());
            if (decl->kind == "class" || decl->kind == "struct") {
                collect_class_decl(decl);
            }
        }
    }

    for (auto& stmt : program->statements) {
        gen_stmt(stmt.get());
    }

    instructions_.emplace_back(vm::Opcode::HLT);
    return instructions_;
}

int CodeGenerator::emit(vm::Opcode op, int a1, int a2, int a3) {
    int pos = static_cast<int>(instructions_.size());
    instructions_.emplace_back(op, a1, a2, a3);
    return pos;
}

int CodeGenerator::emit_jmp(vm::Opcode op, int target) {
    int pos = static_cast<int>(instructions_.size());
    instructions_.emplace_back(op, 0, target, 0);
    return pos;
}

void CodeGenerator::patch(int pos, int target) {
    if (pos >= 0 && pos < (int)instructions_.size()) {
        instructions_[pos].arg2 = target;
    }
}

int CodeGenerator::current_pos() const {
    return static_cast<int>(instructions_.size());
}

static int lookup_var(const std::vector<std::unordered_map<std::string, int>>& stack, const std::string& name) {
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return -1;
}

static std::string required_module_for_function(const std::string& name) {
    if (name == "sin" || name == "cos" || name == "tan" || name == "sqrt" ||
        name == "abs" || name == "floor" || name == "ceil" || name == "pow") {
        return "math";
    }
    return "";
}

int CodeGenerator::emit_condition_jump(ast::Node* condition) {
    if (condition->kind == ast::NodeKind::BinaryExpr) {
        auto* bin = static_cast<ast::BinaryExprNode*>(condition);
        int left_reg = gen_expr(bin->left.get());
        int right_reg = gen_expr(bin->right.get());

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
            default: {
                int zero_reg = next_reg_++;
                emit(vm::Opcode::IMM, zero_reg, 0, 0);
                emit(vm::Opcode::CMP, left_reg, zero_reg, 0);
                return emit_jmp(vm::Opcode::JEQ, 0);
            }
        }
    }

    int reg = gen_expr(condition);
    int zero_reg = next_reg_++;
    emit(vm::Opcode::IMM, zero_reg, 0, 0);
    emit(vm::Opcode::CMP, reg, zero_reg, 0);
    return emit_jmp(vm::Opcode::JEQ, 0);
}

void CodeGenerator::push_scope() {
    var_scope_stack_.push_back({});
}

void CodeGenerator::pop_scope() {
    if (var_scope_stack_.size() > 1) {
        var_scope_stack_.pop_back();
    }
}

int CodeGenerator::declare_var(const std::string& name) {
    auto& top = var_scope_stack_.back();
    auto it = top.find(name);
    if (it != top.end()) {
        return it->second;
    }
    int reg = next_reg_++;
    top[name] = reg;
    return reg;
}

int CodeGenerator::get_var_reg(const std::string& name) {
    return lookup_var(var_scope_stack_, name);
}

void CodeGenerator::collect_class_decl(ast::GenesisDeclNode* decl) {
    if (!decl || (decl->kind != "class" && decl->kind != "struct")) return;
    class_names_.insert(decl->name);
    class_fields_[decl->name].clear();
    field_indices_[decl->name].clear();
    if (!decl->body || decl->body->kind != ast::NodeKind::Block) return;
    auto* block = static_cast<ast::BlockNode*>(decl->body.get());
    int index = 0;
    for (auto& stmt : block->statements) {
        if (stmt->kind == ast::NodeKind::VarDecl) {
            auto* field = static_cast<ast::VarDeclNode*>(stmt.get());
            std::string type = field->type.empty() ? "i32" : field->type;
            class_fields_[decl->name].push_back({field->name, type});
            field_indices_[decl->name][field->name] = index++;
        }
    }
}

void CodeGenerator::generate_function(ast::Node* node) {
    std::string name;
    std::vector<std::pair<std::string, std::string>> params;
    ast::Node* body = nullptr;
    if (node->kind == ast::NodeKind::FuncDef) {
        auto* func = static_cast<ast::FuncDefNode*>(node);
        name = func->name;
        params = func->params;
        body = func->body.get();
    } else if (node->kind == ast::NodeKind::GenesisDecl) {
        auto* decl = static_cast<ast::GenesisDeclNode*>(node);
        if (decl->kind != "func") return;
        name = decl->name;
        params = decl->params;
        body = decl->body.get();
    } else {
        return;
    }

    int saved_next_reg = next_reg_;
    auto saved_scope = var_scope_stack_;
    function_table_[name] = current_pos();
    var_scope_stack_.clear();
    next_reg_ = 0;
    push_scope();
    for (size_t i = 0; i < params.size(); i++) {
        var_scope_stack_.back()[params[i].first] = static_cast<int>(i + 1);
    }
    if (body) gen_stmt(body);
    emit(vm::Opcode::MOV, 0, 0, 0);
    emit(vm::Opcode::RET);
    pop_scope();
    next_reg_ = saved_next_reg;
    var_scope_stack_ = saved_scope;
}

void CodeGenerator::gen_stmt(ast::Node* node) {
    switch (node->kind) {
        case ast::NodeKind::Block: {
            push_scope();
            auto* block = static_cast<ast::BlockNode*>(node);
            for (auto& stmt : block->statements) {
                gen_stmt(stmt.get());
            }
            pop_scope();
            break;
        }
        case ast::NodeKind::VarDecl: {
            auto* decl = static_cast<ast::VarDeclNode*>(node);
            int existing = get_var_reg(decl->name);
            int reg = (existing >= 0) ? existing : declare_var(decl->name);
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

            push_scope();
            int body_start = current_pos();
            gen_stmt(whilestmt->body.get());
            pop_scope();

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
            if (forstmt->bound && forstmt->bound->kind == ast::NodeKind::FuncCall) {
                auto* call = static_cast<ast::FuncCallNode*>(forstmt->bound.get());
                if (call->name == "range" && !call->args.empty()) {
                    bound_reg = gen_expr(call->args[0].get());
                }
            }

            push_scope();
            var_scope_stack_.back()[forstmt->iterator] = iterator_reg;

            emit(vm::Opcode::IMM, iterator_reg, 0, 0);

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

            pop_scope();

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
        case ast::NodeKind::ExprStmt: {
            auto* expr = static_cast<ast::ExprStmtNode*>(node);
            gen_expr(expr->expression.get());
            break;
        }
        case ast::NodeKind::BinaryExpr:
            gen_expr(node);
            break;
        case ast::NodeKind::FuncDef:
            generate_function(node);
            break;
        case ast::NodeKind::GenesisDecl: {
            auto* decl = static_cast<ast::GenesisDeclNode*>(node);
            if (decl->kind == "func") {
                generate_function(decl);
            } else if (decl->kind == "var") {
                int existing = get_var_reg(decl->name);
                int reg = (existing >= 0) ? existing : declare_var(decl->name);
                if (decl->body) {
                    int init_reg = gen_expr(decl->body.get());
                    instructions_.emplace_back(vm::Opcode::MOV, reg, init_reg, 0);
                }
            } else if (decl->kind == "class" || decl->kind == "struct") {
                collect_class_decl(decl);
                if (decl->body && decl->body->kind == ast::NodeKind::Block) {
                    auto* block = static_cast<ast::BlockNode*>(decl->body.get());
                    for (auto& stmt : block->statements) {
                        if (stmt->kind == ast::NodeKind::FuncDef ||
                            (stmt->kind == ast::NodeKind::GenesisDecl && static_cast<ast::GenesisDeclNode*>(stmt.get())->kind == "func")) {
                            std::string method_name = stmt->kind == ast::NodeKind::FuncDef
                                ? static_cast<ast::FuncDefNode*>(stmt.get())->name
                                : static_cast<ast::GenesisDeclNode*>(stmt.get())->name;
                            generate_function(stmt.get());
                            function_table_[method_key(decl->name, method_name)] = function_table_[method_name];
                        }
                    }
                }
            }
            break;
        }
        case ast::NodeKind::IncorporateStmt: {
            auto* inc = static_cast<ast::IncorporateNode*>(node);
            for (const auto& module : inc->modules) {
                incorporate_module(module);
                const auto* mod_ast = module::ModuleLoader::instance().get_module_ast(module);
                if (!mod_ast) continue;
                for (auto& stmt : mod_ast->statements) {
                    if (stmt->kind == ast::NodeKind::GenesisDecl) {
                        auto* decl = static_cast<ast::GenesisDeclNode*>(stmt.get());
                        if (decl->kind == "func") {
                            generate_function(decl);
                        }
                    }
                }
            }
            break;
        }
        case ast::NodeKind::IfStmt: {
            auto* ifstmt = static_cast<ast::IfStmtNode*>(node);
            int if_cond_jump = emit_condition_jump(ifstmt->condition.get());

            push_scope();
            int if_body_start = current_pos();
            gen_stmt(ifstmt->then_block.get());
            pop_scope();
            int if_exit_jump = emit_jmp(vm::Opcode::JMP, 0);

            std::vector<int> elif_cond_jumps;
            std::vector<int> elif_exit_jumps;
            for (auto& [cond, body] : ifstmt->elif_chain) {
                int elif_cond_jump = emit_condition_jump(cond.get());
                elif_cond_jumps.push_back(elif_cond_jump);
                push_scope();
                gen_stmt(body.get());
                pop_scope();
                elif_exit_jumps.push_back(emit_jmp(vm::Opcode::JMP, 0));
            }

            int else_body_start = -1;
            if (ifstmt->else_block) {
                else_body_start = current_pos();
                push_scope();
                gen_stmt(ifstmt->else_block.get());
                pop_scope();
            }

            int after_all = current_pos();

            patch(if_cond_jump, !elif_cond_jumps.empty() ? elif_cond_jumps[0] : (else_body_start >= 0 ? else_body_start : after_all));
            for (size_t i = 0; i < elif_cond_jumps.size(); ++i) {
                int target = (i + 1 < elif_cond_jumps.size()) ? elif_cond_jumps[i + 1] : (else_body_start >= 0 ? else_body_start : after_all);
                patch(elif_cond_jumps[i], target);
            }
            patch(if_exit_jump, after_all);
            for (int j : elif_exit_jumps) patch(j, after_all);
            break;
        }
        default:
            break;
    }
}

int CodeGenerator::gen_lvalue_address(ast::Node* node) {
    if (!node) throw std::runtime_error("Invalid address-of target");
    if (node->kind == ast::NodeKind::Identifier) {
        auto* id = static_cast<ast::IdentifierNode*>(node);
        int reg = get_var_reg(id->name);
        if (reg < 0) reg = declare_var(id->name);
        return reg;
    }
    if (node->kind == ast::NodeKind::MemberExpr) {
        auto* member = static_cast<ast::MemberExprNode*>(node);
        return gen_expr(member->object.get());
    }
    if (node->kind == ast::NodeKind::UnaryExpr) {
        auto* unary = static_cast<ast::UnaryExprNode*>(node);
        if (unary->op == ast::UnaryOp::Deref) {
            return gen_expr(unary->operand.get());
        }
    }
    throw std::runtime_error("Invalid address-of target");
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
            } else if (lit->kind == ast::LiteralKind::Char) {
                instructions_.emplace_back(vm::Opcode::IMM, reg, lit->value.empty() ? 0 : static_cast<unsigned char>(lit->value[0]), 0);
            }
            return reg;
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            if (bin->op == ast::BinaryOp::Assign) {
                int right_reg = gen_expr(bin->right.get());
                if (bin->left->kind == ast::NodeKind::Identifier) {
                    auto* left = static_cast<ast::IdentifierNode*>(bin->left.get());
                    int left_reg = get_var_reg(left->name);
                    if (left_reg < 0) left_reg = declare_var(left->name);
                    instructions_.emplace_back(vm::Opcode::MOV, left_reg, right_reg, 0);
                    return right_reg;
                }
                if (bin->left->kind == ast::NodeKind::MemberExpr) {
                    auto* member = static_cast<ast::MemberExprNode*>(bin->left.get());
                    int obj_reg = gen_expr(member->object.get());
                    if (member->field_index < 0) throw std::runtime_error("Unknown field index for '" + member->field + "'");
                    instructions_.emplace_back(vm::Opcode::SETFIELD, obj_reg, member->field_index, right_reg);
                    return right_reg;
                }
                if (bin->left->kind == ast::NodeKind::UnaryExpr) {
                    auto* unary = static_cast<ast::UnaryExprNode*>(bin->left.get());
                    if (unary->op != ast::UnaryOp::Deref) throw std::runtime_error("Invalid assignment target");
                    int ptr_reg = gen_expr(unary->operand.get());
                    instructions_.emplace_back(vm::Opcode::STORE, ptr_reg, right_reg, 0);
                    return right_reg;
                }
                throw std::runtime_error("Invalid assignment target");
            }
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
                case ast::BinaryOp::Div:
                    instructions_.emplace_back(vm::Opcode::DIV, result_reg, left_reg, right_reg);
                    break;
                case ast::BinaryOp::Mod:
                    instructions_.emplace_back(vm::Opcode::MOD, result_reg, left_reg, right_reg);
                    break;
                case ast::BinaryOp::And: {
                    int zero_reg = next_reg_++;
                    emit(vm::Opcode::IMM, zero_reg, 0, 0);
                    emit(vm::Opcode::CMP, left_reg, zero_reg, 0);
                    int left_zero_jump = emit_jmp(vm::Opcode::JEQ, 0);
                    emit(vm::Opcode::CMP, right_reg, zero_reg, 0);
                    int right_zero_jump = emit_jmp(vm::Opcode::JEQ, 0);
                    int true_pos = current_pos();
                    emit(vm::Opcode::IMM, result_reg, 1, 0);
                    int end_jump = emit_jmp(vm::Opcode::JMP, 0);
                    int zero_pos = current_pos();
                    emit(vm::Opcode::IMM, result_reg, 0, 0);
                    int end_pos = current_pos();
                    patch(left_zero_jump, zero_pos);
                    patch(right_zero_jump, zero_pos);
                    patch(end_jump, end_pos);
                    break;
                }
                case ast::BinaryOp::Or: {
                    int zero_reg = next_reg_++;
                    emit(vm::Opcode::IMM, zero_reg, 0, 0);
                    emit(vm::Opcode::CMP, left_reg, zero_reg, 0);
                    int left_true_jump = emit_jmp(vm::Opcode::JNE, 0);
                    emit(vm::Opcode::CMP, right_reg, zero_reg, 0);
                    int right_true_jump = emit_jmp(vm::Opcode::JNE, 0);
                    int false_pos = current_pos();
                    emit(vm::Opcode::IMM, result_reg, 0, 0);
                    int end_jump = emit_jmp(vm::Opcode::JMP, 0);
                    int true_pos = current_pos();
                    emit(vm::Opcode::IMM, result_reg, 1, 0);
                    int end_pos = current_pos();
                    patch(left_true_jump, true_pos);
                    patch(right_true_jump, true_pos);
                    patch(end_jump, end_pos);
                    (void)false_pos;
                    break;
                }
                default:
                    break;
            }
            return result_reg;
        }
        case ast::NodeKind::UnaryExpr: {
            auto* unary = static_cast<ast::UnaryExprNode*>(node);
            if (unary->op == ast::UnaryOp::AddressOf) {
                return gen_lvalue_address(unary->operand.get());
            }
            if (unary->op == ast::UnaryOp::Deref) {
                int ptr_reg = gen_expr(unary->operand.get());
                int result_reg = next_reg_++;
                instructions_.emplace_back(vm::Opcode::LOAD, ptr_reg, result_reg, 0);
                return result_reg;
            }
            int operand_reg = gen_expr(unary->operand.get());
            int result_reg = next_reg_++;
            if (unary->op == ast::UnaryOp::Neg) {
                int zero_reg = next_reg_++;
                emit(vm::Opcode::IMM, zero_reg, 0, 0);
                emit(vm::Opcode::SUB, result_reg, zero_reg, operand_reg);
            } else {
                int zero_reg = next_reg_++;
                emit(vm::Opcode::IMM, zero_reg, 0, 0);
                emit(vm::Opcode::CMP, operand_reg, zero_reg, 0);
                int nonzero_jump = emit_jmp(vm::Opcode::JNE, 0);
                emit(vm::Opcode::IMM, result_reg, 1, 0);
                int end_jump = emit_jmp(vm::Opcode::JMP, 0);
                int false_pos = current_pos();
                emit(vm::Opcode::IMM, result_reg, 0, 0);
                int end_pos = current_pos();
                patch(nonzero_jump, false_pos);
                patch(end_jump, end_pos);
            }
            return result_reg;
        }
        case ast::NodeKind::ArrayLit: {
            auto* arr = static_cast<ast::ArrayLitNode*>(node);
            int size_reg = next_reg_++;
            emit(vm::Opcode::IMM, size_reg, static_cast<int>(arr->elements.size()), 0);
            int result_reg = next_reg_++;
            emit(vm::Opcode::ARR_NEW, result_reg, size_reg, 0);
            for (size_t i = 0; i < arr->elements.size(); i++) {
                int elem_reg = gen_expr(arr->elements[i].get());
                emit(vm::Opcode::ARR_SET, result_reg, static_cast<int>(i), elem_reg);
            }
            return result_reg;
        }
        case ast::NodeKind::IndexExpr: {
            auto* idx = static_cast<ast::IndexExprNode*>(node);
            int obj_reg = gen_expr(idx->object.get());
            int index_reg = gen_expr(idx->index.get());
            int result_reg = next_reg_++;
            emit(vm::Opcode::ARR_GET, obj_reg, index_reg, result_reg);
            return result_reg;
        }
        case ast::NodeKind::Identifier: {
            auto* id = static_cast<ast::IdentifierNode*>(node);
            int reg = get_var_reg(id->name);
            if (reg >= 0) return reg;
            int r = next_reg_++;
            instructions_.emplace_back(vm::Opcode::IMM, r, 0, 0);
            return r;
        }
        case ast::NodeKind::MemberExpr: {
            auto* member = static_cast<ast::MemberExprNode*>(node);
            int obj_reg = gen_expr(member->object.get());
            if (member->field_index < 0) throw std::runtime_error("Unknown field index for '" + member->field + "'");
            int result_reg = next_reg_++;
            instructions_.emplace_back(vm::Opcode::GETFIELD, obj_reg, member->field_index, result_reg);
            return result_reg;
        }
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            if (class_names_.find(call->name) != class_names_.end()) {
                int result_reg = next_reg_++;
                int class_idx = add_string(call->name);
                auto fields_it = class_fields_.find(call->name);
                int field_count = fields_it == class_fields_.end() ? 0 : static_cast<int>(fields_it->second.size());
                instructions_.emplace_back(vm::Opcode::NEW, result_reg, class_idx, field_count);
                for (size_t i = 0; i < call->args.size(); i++) {
                    int arg_reg = gen_expr(call->args[i].get());
                    instructions_.emplace_back(vm::Opcode::SETFIELD, result_reg, static_cast<int>(i), arg_reg);
                }
                return result_reg;
            }

            if (call->receiver) {
                int result_reg = next_reg_++;
                int receiver_reg = gen_expr(call->receiver.get());
                instructions_.emplace_back(vm::Opcode::MOV, 1, receiver_reg, 0);
                for (size_t i = 0; i < call->args.size() && i < 14; i++) {
                    int arg_reg = gen_expr(call->args[i].get());
                    instructions_.emplace_back(vm::Opcode::MOV, static_cast<int>(i + 2), arg_reg, 0);
                }
                std::string receiver_base = base_type(call->receiver_type);
                std::string key = method_key(receiver_base, call->name);
                auto it = function_table_.find(key);
                int func_idx = it != function_table_.end() ? it->second : add_string(call->name);
                instructions_.emplace_back(vm::Opcode::CALL, result_reg, func_idx, static_cast<int>(call->args.size() + 1));
                return result_reg;
            }

            if (!is_module_function_available(call->name)) {
                throw std::runtime_error("Module function '" + call->name + "()' requires Incorporate '" + required_module_for_function(call->name) + "'");
            }
            int result_reg = next_reg_++;
            auto it = function_table_.find(call->name);
            if (it != function_table_.end()) {
                int func_start = it->second;
                for (size_t i = 0; i < call->args.size() && i < 15; i++) {
                    int arg_reg = gen_expr(call->args[i].get());
                    instructions_.emplace_back(vm::Opcode::MOV, static_cast<int>(i + 1), arg_reg, 0);
                }
                instructions_.emplace_back(vm::Opcode::CALL, result_reg, func_start, static_cast<int>(call->args.size()));
            } else {
                int func_idx = add_string(call->name);
                for (size_t i = 0; i < call->args.size() && i < 15; i++) {
                    int arg_reg = gen_expr(call->args[i].get());
                    instructions_.emplace_back(vm::Opcode::MOV, static_cast<int>(i + 1), arg_reg, 0);
                }
                instructions_.emplace_back(vm::Opcode::CALL, result_reg, func_idx, static_cast<int>(call->args.size()));
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

} // namespace codegen
