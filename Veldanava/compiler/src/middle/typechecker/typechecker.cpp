#include "typechecker.h"
#include "ast/ast.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <algorithm>

namespace typechecker {

static bool starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

static bool is_pointer_type(const std::string& type) {
    return starts_with(type, "ptr ");
}

static bool is_reference_type(const std::string& type) {
    return starts_with(type, "ref ");
}

static std::string pointee_type(const std::string& type) {
    if (is_pointer_type(type) || is_reference_type(type)) {
        return type.substr(4);
    }
    return "";
}

static std::string base_type(const std::string& type) {
    std::string current = type;
    while (is_pointer_type(current) || is_reference_type(current)) {
        current = pointee_type(current);
    }
    return current;
}

static bool is_primitive_type(const std::string& type) {
    return type == "i32" || type == "i64" || type == "u32" || type == "u64" ||
           type == "f32" || type == "f64" || type == "bool" || type == "string" ||
           type == "char" || type == "void";
}

static bool is_numeric_type(const std::string& type) {
    return type == "i32" || type == "i64" || type == "u32" || type == "u64" ||
           type == "f32" || type == "f64";
}

static bool is_known_type(const TypeContext& ctx, const std::string& type) {
    if (is_primitive_type(type)) return true;
    if (ctx.class_names.find(type) != ctx.class_names.end()) return true;
    if (is_pointer_type(type) || is_reference_type(type)) {
        return is_known_type(ctx, pointee_type(type));
    }
    return false;
}

static bool is_compatible(const std::string& expected, const std::string& actual) {
    if (expected == actual) return true;
    if (expected.empty() || actual == "void") return true;
    if (is_numeric_type(expected) && is_numeric_type(actual)) return true;
    if (is_reference_type(expected) && is_pointer_type(actual) && pointee_type(expected) == pointee_type(actual)) return true;
    return false;
}

static std::string method_key(const std::string& class_name, const std::string& method_name) {
    return class_name + "::" + method_name;
}

static void collect_class_body(TypeContext& ctx, const std::string& class_name, ast::Node* body) {
    if (!body || body->kind != ast::NodeKind::Block) return;
    auto* block = static_cast<ast::BlockNode*>(body);
    for (auto& stmt : block->statements) {
        if (stmt->kind == ast::NodeKind::VarDecl) {
            auto* decl = static_cast<ast::VarDeclNode*>(stmt.get());
            std::string field_type = decl->type.empty() ? "i32" : decl->type;
            ctx.class_field_order[class_name].push_back({decl->name, field_type});
            ctx.class_fields[class_name + "." + decl->name] = field_type;
        } else if (stmt->kind == ast::NodeKind::FuncDef || stmt->kind == ast::NodeKind::GenesisDecl) {
            auto* decl = static_cast<ast::GenesisDeclNode*>(stmt.get());
            if (decl->kind == "func") {
                FunctionInfo info;
                info.params = decl->params;
                info.return_type = decl->return_type.empty() ? "i32" : decl->return_type;
                ctx.class_methods[class_name][decl->name] = info;
            }
        }
    }
}

static void collect_declarations(ast::ProgramNode* program, TypeContext& ctx) {
    if (!program) return;
    for (auto& stmt : program->statements) {
        if (stmt->kind == ast::NodeKind::GenesisDecl) {
            auto* decl = static_cast<ast::GenesisDeclNode*>(stmt.get());
            if (decl->kind == "func") {
                FunctionInfo info;
                info.params = decl->params;
                info.return_type = decl->return_type.empty() ? "i32" : decl->return_type;
                ctx.functions[decl->name] = info;
            } else if (decl->kind == "class" || decl->kind == "struct") {
                ctx.class_names.insert(decl->name);
                collect_class_body(ctx, decl->name, decl->body.get());
            }
        }
    }
}

static std::string infer_type(ast::Node* node, TypeContext& ctx);

static bool is_lvalue(ast::Node* node) {
    if (!node) return false;
    if (node->kind == ast::NodeKind::UnaryExpr) {
        auto* unary = static_cast<ast::UnaryExprNode*>(node);
        return unary->op == ast::UnaryOp::Deref;
    }
    return node->kind == ast::NodeKind::Identifier ||
           node->kind == ast::NodeKind::MemberExpr;
}

static void validate_expression(ast::Node* node, TypeContext& ctx) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            validate_expression(bin->left.get(), ctx);
            validate_expression(bin->right.get(), ctx);
            if (bin->op == ast::BinaryOp::Assign) {
                std::string left_type = infer_type(bin->left.get(), ctx);
                std::string right_type = infer_type(bin->right.get(), ctx);
                if (!is_compatible(left_type, right_type)) {
                    ctx.errors.push_back("Type mismatch: cannot assign " + right_type + " to " + left_type);
                }
            } else {
                (void)infer_type(bin, ctx);
            }
            break;
        }
        case ast::NodeKind::UnaryExpr: {
            auto* unary = static_cast<ast::UnaryExprNode*>(node);
            validate_expression(unary->operand.get(), ctx);
            if (unary->op == ast::UnaryOp::AddressOf && !is_lvalue(unary->operand.get())) {
                ctx.errors.push_back("Address-of requires an lvalue");
            }
            if (unary->op == ast::UnaryOp::Deref) {
                std::string operand_type = infer_type(unary->operand.get(), ctx);
                if (!is_pointer_type(operand_type) && !is_reference_type(operand_type)) {
                    ctx.errors.push_back("Deref requires pointer or reference, got " + operand_type);
                }
            }
            break;
        }
        case ast::NodeKind::MemberExpr: {
            auto* member = static_cast<ast::MemberExprNode*>(node);
            validate_expression(member->object.get(), ctx);
            (void)infer_type(member, ctx);
            break;
        }
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            validate_expression(call->receiver.get(), ctx);
            for (auto& arg : call->args) validate_expression(arg.get(), ctx);
            (void)infer_type(call, ctx);
            break;
        }
        default:
            (void)infer_type(node, ctx);
            break;
    }
}

static std::string infer_type(ast::Node* node, TypeContext& ctx) {
    if (!node) return "void";
    switch (node->kind) {
        case ast::NodeKind::Literal: {
            auto* lit = static_cast<ast::LiteralNode*>(node);
            switch (lit->kind) {
                case ast::LiteralKind::Int: return "i32";
                case ast::LiteralKind::Bool: return "bool";
                case ast::LiteralKind::String: return "string";
                case ast::LiteralKind::Float: return "f32";
                case ast::LiteralKind::Char: return "char";
            }
            break;
        }
        case ast::NodeKind::Identifier: {
            auto* id = static_cast<ast::IdentifierNode*>(node);
            auto it = ctx.variables.find(id->name);
            return it == ctx.variables.end() ? "void" : it->second;
        }
        case ast::NodeKind::MemberExpr: {
            auto* member = static_cast<ast::MemberExprNode*>(node);
            std::string object_type = infer_type(member->object.get(), ctx);
            std::string base = base_type(object_type);
            auto field_it = ctx.class_fields.find(base + "." + member->field);
            if (field_it == ctx.class_fields.end()) {
                ctx.errors.push_back("Unknown field '" + member->field + "' on type " + base);
                return "void";
            }
            member->field_type = field_it->second;
            int index = 0;
            auto order_it = ctx.class_field_order.find(base);
            if (order_it != ctx.class_field_order.end()) {
                for (const auto& [name, type] : order_it->second) {
                    if (name == member->field) {
                        member->field_index = index;
                        break;
                    }
                    index++;
                }
            }
            return member->field_type;
        }
        case ast::NodeKind::UnaryExpr: {
            auto* unary = static_cast<ast::UnaryExprNode*>(node);
            if (unary->op == ast::UnaryOp::AddressOf) {
                std::string operand_type = infer_type(unary->operand.get(), ctx);
                return "ptr " + operand_type;
            }
            if (unary->op == ast::UnaryOp::Deref) {
                std::string operand_type = infer_type(unary->operand.get(), ctx);
                if (is_pointer_type(operand_type) || is_reference_type(operand_type)) {
                    return pointee_type(operand_type);
                }
                ctx.errors.push_back("Deref requires pointer or reference, got " + operand_type);
                return "void";
            }
            break;
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            if (bin->op == ast::BinaryOp::Assign) return infer_type(bin->right.get(), ctx);
            std::string left = infer_type(bin->left.get(), ctx);
            std::string right = infer_type(bin->right.get(), ctx);
            switch (bin->op) {
                case ast::BinaryOp::Lt:
                case ast::BinaryOp::Gt:
                case ast::BinaryOp::Le:
                case ast::BinaryOp::Ge:
                case ast::BinaryOp::Eq:
                case ast::BinaryOp::Ne:
                case ast::BinaryOp::And:
                case ast::BinaryOp::Or:
                    return "bool";
                case ast::BinaryOp::Add:
                case ast::BinaryOp::Sub:
                case ast::BinaryOp::Mul:
                case ast::BinaryOp::Div:
                case ast::BinaryOp::Mod:
                    if (is_numeric_type(left) && is_numeric_type(right)) return "i64";
                    return "i32";
                default:
                    break;
            }
            break;
        }
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            if (call->receiver) {
                std::string receiver_type = infer_type(call->receiver.get(), ctx);
                std::string base = base_type(receiver_type);
                call->receiver_type = receiver_type;
                auto class_it = ctx.class_methods.find(base);
                if (class_it == ctx.class_methods.end()) {
                    ctx.errors.push_back("Method call on non-class type " + base);
                    return "void";
                }
                auto method_it = class_it->second.find(call->name);
                if (method_it == class_it->second.end()) {
                    ctx.errors.push_back("Unknown method '" + call->name + "' on type " + base);
                    return "void";
                }
                const FunctionInfo& method = method_it->second;
                int expected_args = static_cast<int>(method.params.size()) - 1;
                if (expected_args < 0 || static_cast<int>(call->args.size()) != expected_args) {
                    ctx.errors.push_back("Method '" + call->name + "' expects " + std::to_string(expected_args) + " argument(s), got " + std::to_string(call->args.size()));
                    return method.return_type;
                }
                if (!method.params.empty() && !is_compatible(method.params[0].second, receiver_type)) {
                    ctx.errors.push_back("Method receiver type mismatch: expected " + method.params[0].second + ", got " + receiver_type);
                }
                for (size_t i = 0; i < call->args.size(); i++) {
                    std::string arg_type = infer_type(call->args[i].get(), ctx);
                    std::string expected = i + 1 < method.params.size() ? method.params[i + 1].second : "void";
                    if (!is_compatible(expected, arg_type)) {
                        ctx.errors.push_back("Method argument type mismatch: expected " + expected + ", got " + arg_type);
                    }
                }
                return method.return_type;
            }

            if (ctx.class_names.find(call->name) != ctx.class_names.end()) {
                auto order_it = ctx.class_field_order.find(call->name);
                size_t field_count = order_it == ctx.class_field_order.end() ? 0 : order_it->second.size();
                if (call->args.size() > field_count) {
                    ctx.errors.push_back("Constructor '" + call->name + "' expects at most " + std::to_string(field_count) + " argument(s), got " + std::to_string(call->args.size()));
                }
                for (size_t i = 0; i < call->args.size() && i < field_count; i++) {
                    std::string expected = order_it->second[i].second;
                    std::string actual = infer_type(call->args[i].get(), ctx);
                    if (!is_compatible(expected, actual)) {
                        ctx.errors.push_back("Constructor argument type mismatch: expected " + expected + ", got " + actual);
                    }
                }
                return call->name;
            }

            auto func_it = ctx.functions.find(call->name);
            if (func_it != ctx.functions.end()) {
                const FunctionInfo& fn = func_it->second;
                if (fn.params.size() != call->args.size()) {
                    ctx.errors.push_back("Function '" + call->name + "' expects " + std::to_string(fn.params.size()) + " argument(s), got " + std::to_string(call->args.size()));
                }
                for (size_t i = 0; i < call->args.size() && i < fn.params.size(); i++) {
                    std::string expected = fn.params[i].second;
                    std::string actual = infer_type(call->args[i].get(), ctx);
                    if (!is_compatible(expected, actual)) {
                        ctx.errors.push_back("Function argument type mismatch: expected " + expected + ", got " + actual);
                    }
                }
                return fn.return_type;
            }

            if (call->name == "print" || call->name == "scribe") return "void";
            if (call->name == "range") return "i32";
            return "i32";
        }
        default:
            break;
    }
    return "void";
}

static void mark_mutation(ast::Node* node, TypeContext& ctx) {
    if (!node) return;
    if (node->kind == ast::NodeKind::Identifier) {
        auto* id = static_cast<ast::IdentifierNode*>(node);
        auto mutable_it = ctx.mutable_vars.find(id->name);
        if (mutable_it != ctx.mutable_vars.end() && !mutable_it->second) {
            ctx.errors.push_back("Cannot assign to const variable '" + id->name + "'");
        }
        auto borrow_it = ctx.borrow_count.find(id->name);
        if (borrow_it != ctx.borrow_count.end() && borrow_it->second > 0) {
            ctx.errors.push_back("Cannot assign to borrowed variable '" + id->name + "'");
        }
    } else if (node->kind == ast::NodeKind::UnaryExpr) {
        auto* unary = static_cast<ast::UnaryExprNode*>(node);
        if (unary->op != ast::UnaryOp::Deref) return;
        std::string ptr_type = infer_type(unary->operand.get(), ctx);
        if (is_reference_type(ptr_type)) {
            ctx.errors.push_back("Cannot assign through immutable reference");
        }
    } else if (node->kind == ast::NodeKind::MemberExpr) {
        auto* member = static_cast<ast::MemberExprNode*>(node);
        if (member->object && member->object->kind == ast::NodeKind::Identifier) {
            auto* id = static_cast<ast::IdentifierNode*>(member->object.get());
            auto borrow_it = ctx.borrow_count.find(id->name);
            if (borrow_it != ctx.borrow_count.end() && borrow_it->second > 0) {
                ctx.errors.push_back("Cannot mutate borrowed object '" + id->name + "'");
            }
        }
    }
}

static void validate_statement(ast::Node* node, TypeContext& ctx) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::VarDecl: {
            auto* decl = static_cast<ast::VarDeclNode*>(node);
            if (decl->initializer) {
                validate_expression(decl->initializer.get(), ctx);
            }
            std::string declared_type = decl->type;
            if (!declared_type.empty()) {
                ctx.variables[decl->name] = declared_type;
                ctx.mutable_vars[decl->name] = decl->is_mutable;
            }
            if (decl->initializer) {
                validate_expression(decl->initializer.get(), ctx);
            }
            std::string inferred_type = decl->initializer ? infer_type(decl->initializer.get(), ctx) : "void";
            if (declared_type.empty()) {
                declared_type = inferred_type;
            }
            if (!declared_type.empty() && !is_known_type(ctx, declared_type)) {
                ctx.errors.push_back("Unknown type: " + declared_type);
            }
            if (decl->initializer && !is_compatible(declared_type, inferred_type)) {
                ctx.errors.push_back("Type mismatch: cannot assign " + inferred_type + " to " + declared_type);
            }
            ctx.variables[decl->name] = declared_type;
            ctx.mutable_vars[decl->name] = decl->is_mutable;
            break;
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            validate_expression(bin, ctx);
            if (bin->op == ast::BinaryOp::Assign) {
                mark_mutation(bin->left.get(), ctx);
            }
            break;
        }
        case ast::NodeKind::Block: {
            auto* block = static_cast<ast::BlockNode*>(node);
            for (auto& stmt : block->statements) {
                validate_statement(stmt.get(), ctx);
            }
            break;
        }
        case ast::NodeKind::IfStmt: {
            auto* stmt = static_cast<ast::IfStmtNode*>(node);
            validate_expression(stmt->condition.get(), ctx);
            validate_statement(stmt->then_block.get(), ctx);
            for (auto& [cond, body] : stmt->elif_chain) {
                validate_expression(cond.get(), ctx);
                std::string elif_type = infer_type(cond.get(), ctx);
                if (elif_type != "bool") {
                    ctx.errors.push_back("Elif condition must be bool, got " + elif_type);
                }
                validate_statement(body.get(), ctx);
            }
            if (stmt->else_block) {
                validate_statement(stmt->else_block.get(), ctx);
            }
            break;
        }
        case ast::NodeKind::WhileStmt: {
            auto* stmt = static_cast<ast::WhileStmtNode*>(node);
            validate_expression(stmt->condition.get(), ctx);
            validate_statement(stmt->body.get(), ctx);
            break;
        }
        case ast::NodeKind::ForStmt: {
            auto* stmt = static_cast<ast::ForStmtNode*>(node);
            validate_expression(stmt->bound.get(), ctx);
            validate_statement(stmt->body.get(), ctx);
            break;
        }
        case ast::NodeKind::FuncDef:
        case ast::NodeKind::GenesisDecl: {
            auto* decl = static_cast<ast::GenesisDeclNode*>(node);
            if (decl->kind == "class" || decl->kind == "struct") {
                if (decl->body && decl->body->kind == ast::NodeKind::Block) {
                    auto* block = static_cast<ast::BlockNode*>(decl->body.get());
                    for (auto& stmt : block->statements) {
                        if (stmt->kind == ast::NodeKind::FuncDef ||
                            (stmt->kind == ast::NodeKind::GenesisDecl && static_cast<ast::GenesisDeclNode*>(stmt.get())->kind == "func")) {
                            auto saved_vars = ctx.variables;
                            auto saved_mutable = ctx.mutable_vars;
                            std::string method_return = "i32";
                            std::vector<std::pair<std::string, std::string>> params;
                            ast::Node* body = nullptr;
                            if (stmt->kind == ast::NodeKind::FuncDef) {
                                auto* func = static_cast<ast::FuncDefNode*>(stmt.get());
                                method_return = func->return_type.empty() ? "i32" : func->return_type;
                                params = func->params;
                                body = func->body.get();
                            } else {
                                auto* func = static_cast<ast::GenesisDeclNode*>(stmt.get());
                                method_return = func->return_type.empty() ? "i32" : func->return_type;
                                params = func->params;
                                body = func->body.get();
                            }
                            ctx.current_function_return_type = method_return;
                            for (const auto& [name, type] : params) {
                                ctx.variables[name] = type;
                                ctx.mutable_vars[name] = true;
                            }
                            if (body) validate_statement(body, ctx);
                            ctx.variables = saved_vars;
                            ctx.mutable_vars = saved_mutable;
                            ctx.current_function_return_type.clear();
                        }
                    }
                }
                break;
            }
            if (decl->kind == "func") {
                auto saved_vars = ctx.variables;
                auto saved_mutable = ctx.mutable_vars;
                ctx.current_function_return_type = decl->return_type.empty() ? "i32" : decl->return_type;
                for (const auto& [name, type] : decl->params) {
                    if (!is_known_type(ctx, type)) {
                        ctx.errors.push_back("Unknown parameter type: " + type);
                    }
                    ctx.variables[name] = type;
                    ctx.mutable_vars[name] = true;
                }
                if (decl->body) {
                    validate_statement(decl->body.get(), ctx);
                }
                ctx.variables = saved_vars;
                ctx.mutable_vars = saved_mutable;
                ctx.current_function_return_type.clear();
            }
            break;
        }
        case ast::NodeKind::ReturnStmt: {
            auto* ret = static_cast<ast::ReturnStmtNode*>(node);
            if (ret->value) {
                validate_expression(ret->value.get(), ctx);
            }
            if (!ctx.current_function_return_type.empty() && ctx.current_function_return_type != "void") {
                std::string ret_type = ret->value ? infer_type(ret->value.get(), ctx) : "void";
                if (!is_compatible(ctx.current_function_return_type, ret_type)) {
                    ctx.errors.push_back("Return type mismatch: expected " + ctx.current_function_return_type + ", got " + ret_type);
                }
            }
            break;
        }
        case ast::NodeKind::ExprStmt:
            validate_statement(static_cast<ast::ExprStmtNode*>(node)->expression.get(), ctx);
            break;
        case ast::NodeKind::FuncCall:
        case ast::NodeKind::Identifier:
        case ast::NodeKind::MemberExpr:
        case ast::NodeKind::UnaryExpr:
            validate_expression(node, ctx);
            break;
        default:
            break;
    }
}

std::vector<std::string> type_check(ast::ProgramNode* program) {
    TypeContext ctx;
    collect_declarations(program, ctx);
    for (auto& stmt : program->statements) {
        validate_statement(stmt.get(), ctx);
    }
    return ctx.errors;
}

}
