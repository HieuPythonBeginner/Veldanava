#include "typechecker.h"
#include "ast/ast.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace typechecker {

static bool is_primitive_type(const std::string& type) {
    return type == "i32" || type == "i64" || type == "u32" || type == "u64" ||
           type == "f32" || type == "f64" || type == "bool" || type == "string" ||
           type == "char" || type == "void";
}

static std::string infer_type(ast::Node* node) {
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
            return id->name;
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            std::string left = infer_type(bin->left.get());
            std::string right = infer_type(bin->right.get());
            if (left == right) return left;
            if ((left == "i32" || left == "i64" || left == "u32" || left == "u64") &&
                (right == "i32" || right == "i64" || right == "u32" || right == "u64")) {
                return "i64";
            }
            return "i32";
        }
        case ast::NodeKind::UnaryExpr: {
            auto* unary = static_cast<ast::UnaryExprNode*>(node);
            return infer_type(unary->operand.get());
        }
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            return "i32";
        }
        default:
            break;
    }
    return "void";
}

static void validate_statement(ast::Node* node, TypeContext& ctx) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::VarDecl: {
            auto* decl = static_cast<ast::VarDeclNode*>(node);
            if (!decl->type.empty() && !is_primitive_type(decl->type)) {
                ctx.errors.push_back("Unknown type: " + decl->type);
            }
            if (decl->initializer) {
                std::string init_type = infer_type(decl->initializer.get());
                if (!decl->type.empty() && init_type != decl->type && !(decl->type == "i32" && init_type == "i32")) {
                    ctx.errors.push_back("Type mismatch: cannot assign " + init_type + " to " + decl->type);
                }
                ctx.variables[decl->name] = decl->type;
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
            std::string cond_type = infer_type(stmt->condition.get());
            if (cond_type != "bool") {
                ctx.errors.push_back("If condition must be bool, got " + cond_type);
            }
            validate_statement(stmt->then_block.get(), ctx);
            for (auto& [cond, body] : stmt->elif_chain) {
                std::string elif_type = infer_type(cond.get());
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
            std::string cond_type = infer_type(stmt->condition.get());
            if (cond_type != "bool") {
                ctx.errors.push_back("While condition must be bool, got " + cond_type);
            }
            validate_statement(stmt->body.get(), ctx);
            break;
        }
        case ast::NodeKind::ForStmt: {
            auto* stmt = static_cast<ast::ForStmtNode*>(node);
            validate_statement(stmt->body.get(), ctx);
            break;
        }
        case ast::NodeKind::FuncDef:
        case ast::NodeKind::GenesisDecl: {
            auto* decl = static_cast<ast::GenesisDeclNode*>(node);
            if (!decl->return_type.empty() && !is_primitive_type(decl->return_type)) {
                ctx.errors.push_back("Unknown return type: " + decl->return_type);
            }
            ctx.current_function_return_type = decl->return_type;
            if (decl->body) {
                validate_statement(decl->body.get(), ctx);
            }
            ctx.current_function_return_type.clear();
            break;
        }
        case ast::NodeKind::ReturnStmt: {
            auto* ret = static_cast<ast::ReturnStmtNode*>(node);
            if (!ctx.current_function_return_type.empty()) {
                std::string ret_type = infer_type(ret->value.get());
                if (ret_type != ctx.current_function_return_type) {
                    ctx.errors.push_back("Return type mismatch: expected " + ctx.current_function_return_type + ", got " + ret_type);
                }
            }
            break;
        }
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            for (auto& arg : call->args) {
                validate_statement(arg.get(), ctx);
            }
            break;
        }
        default:
            break;
    }
}

std::vector<std::string> type_check(ast::ProgramNode* program) {
    TypeContext ctx;
    for (auto& stmt : program->statements) {
        validate_statement(stmt.get(), ctx);
    }
    return ctx.errors;
}

}
