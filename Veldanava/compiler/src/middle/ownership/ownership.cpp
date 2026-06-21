#include "ownership.h"
#include "ast/ast.h"
#include <unordered_map>
#include <string>
#include <stdexcept>

namespace ownership {

struct VarInfo {
    std::string type;
    bool is_mutable = true;
    int borrow_count = 0;
};

struct OwnershipState {
    std::unordered_map<std::string, VarInfo> variables;
    int current_scope = 0;
};

static OwnershipState state;

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
    return type;
}

static std::string type_of(ast::Node* node) {
    if (!node) return "void";
    switch (node->kind) {
        case ast::NodeKind::Identifier: {
            auto* id = static_cast<ast::IdentifierNode*>(node);
            auto it = state.variables.find(id->name);
            return it == state.variables.end() ? "void" : it->second.type;
        }
        case ast::NodeKind::UnaryExpr: {
            auto* unary = static_cast<ast::UnaryExprNode*>(node);
            if (unary->op == ast::UnaryOp::AddressOf) {
                return "ptr " + type_of(unary->operand.get());
            }
            if (unary->op == ast::UnaryOp::Deref) {
                return pointee_type(type_of(unary->operand.get()));
            }
            return "void";
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            if (bin->op == ast::BinaryOp::Assign) return type_of(bin->right.get());
            return "i32";
        }
        case ast::NodeKind::FuncCall:
            return "i32";
        default:
            return "void";
    }
}

static bool is_lvalue(ast::Node* node) {
    if (!node) return false;
    if (node->kind == ast::NodeKind::UnaryExpr) {
        auto* unary = static_cast<ast::UnaryExprNode*>(node);
        return unary->op == ast::UnaryOp::Deref;
    }
    return node->kind == ast::NodeKind::Identifier ||
           node->kind == ast::NodeKind::MemberExpr;
}

static std::string lvalue_name(ast::Node* node) {
    if (!node) return "";
    if (node->kind == ast::NodeKind::Identifier) {
        return static_cast<ast::IdentifierNode*>(node)->name;
    }
    if (node->kind == ast::NodeKind::MemberExpr) {
        auto* member = static_cast<ast::MemberExprNode*>(node);
        return lvalue_name(member->object.get());
    }
    if (node->kind == ast::NodeKind::UnaryExpr) {
        auto* unary = static_cast<ast::UnaryExprNode*>(node);
        if (unary->op == ast::UnaryOp::Deref) {
            return lvalue_name(unary->operand.get());
        }
        return "";
    }
    return "";
}

static void validate_node(ast::Node* node);

static void validate_expression(ast::Node* node) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            validate_expression(bin->left.get());
            validate_expression(bin->right.get());
            if (bin->op == ast::BinaryOp::Assign) {
                if (!is_lvalue(bin->left.get())) {
                    throw std::runtime_error("ownership: Invalid assignment target");
                }
                std::string name = lvalue_name(bin->left.get());
                auto it = state.variables.find(name);
                if (it != state.variables.end() && it->second.borrow_count > 0) {
                    throw std::runtime_error("Cannot assign to borrowed variable '" + name + "'");
                }
                if (bin->left->kind == ast::NodeKind::UnaryExpr) {
                    auto* unary = static_cast<ast::UnaryExprNode*>(bin->left.get());
                    if (unary->op == ast::UnaryOp::Deref && is_reference_type(type_of(unary->operand.get()))) {
                        throw std::runtime_error("ownership: Cannot assign through immutable reference");
                    }
                }
            }
            break;
        }
        case ast::NodeKind::UnaryExpr: {
            auto* unary = static_cast<ast::UnaryExprNode*>(node);
            validate_expression(unary->operand.get());
            if (unary->op == ast::UnaryOp::AddressOf) {
                if (!is_lvalue(unary->operand.get())) {
                    throw std::runtime_error("Address-of requires an lvalue");
                }
                std::string name = lvalue_name(unary->operand.get());
                if (!name.empty()) {
                    state.variables[name].borrow_count++;
                }
            }
            if (unary->op == ast::UnaryOp::Deref && is_reference_type(type_of(unary->operand.get()))) {
                throw std::runtime_error("Cannot dereference immutable reference as mutable");
            }
            break;
        }
        case ast::NodeKind::MemberExpr:
            validate_expression(static_cast<ast::MemberExprNode*>(node)->object.get());
            break;
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            validate_expression(call->receiver.get());
            for (auto& arg : call->args) validate_expression(arg.get());
            break;
        }
        default:
            break;
    }
}

static void validate_statement(ast::Node* node) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::VarDecl: {
            auto* decl = static_cast<ast::VarDeclNode*>(node);
            if (decl->initializer) validate_expression(decl->initializer.get());
            state.variables[decl->name] = {decl->type.empty() ? (decl->initializer ? type_of(decl->initializer.get()) : "void") : decl->type, decl->is_mutable, 0};
            break;
        }
        case ast::NodeKind::BinaryExpr:
            validate_expression(node);
            break;
        case ast::NodeKind::Block: {
            state.current_scope++;
            auto* block = static_cast<ast::BlockNode*>(node);
            for (auto& stmt : block->statements) validate_statement(stmt.get());
            state.current_scope--;
            break;
        }
        case ast::NodeKind::IfStmt: {
            auto* stmt = static_cast<ast::IfStmtNode*>(node);
            validate_expression(stmt->condition.get());
            state.current_scope++;
            validate_statement(stmt->then_block.get());
            state.current_scope--;
            if (stmt->else_block) {
                state.current_scope++;
                validate_statement(stmt->else_block.get());
                state.current_scope--;
            }
            for (auto& [cond, body] : stmt->elif_chain) {
                validate_expression(cond.get());
                state.current_scope++;
                validate_statement(body.get());
                state.current_scope--;
            }
            break;
        }
        case ast::NodeKind::WhileStmt: {
            auto* stmt = static_cast<ast::WhileStmtNode*>(node);
            validate_expression(stmt->condition.get());
            state.current_scope++;
            validate_statement(stmt->body.get());
            state.current_scope--;
            break;
        }
        case ast::NodeKind::ForStmt: {
            auto* stmt = static_cast<ast::ForStmtNode*>(node);
            validate_expression(stmt->bound.get());
            state.current_scope++;
            validate_statement(stmt->body.get());
            state.current_scope--;
            break;
        }
        case ast::NodeKind::FuncDef:
        case ast::NodeKind::GenesisDecl: {
            auto* decl = static_cast<ast::GenesisDeclNode*>(node);
            if (decl->kind == "class" || decl->kind == "struct") break;
            state.current_scope++;
            for (const auto& [name, type] : decl->params) {
                state.variables[name] = {type, true, 0};
            }
            if (decl->body) validate_statement(decl->body.get());
            state.current_scope--;
            break;
        }
        case ast::NodeKind::ExprStmt:
            validate_statement(static_cast<ast::ExprStmtNode*>(node)->expression.get());
            break;
        case ast::NodeKind::FuncCall:
        case ast::NodeKind::Identifier:
        case ast::NodeKind::MemberExpr:
        case ast::NodeKind::UnaryExpr:
            validate_expression(node);
            break;
        default:
            break;
    }
}

void init() {
    state.variables.clear();
    state.current_scope = 0;
}

void enter_scope() {
    state.current_scope++;
}

void exit_scope() {
    state.current_scope--;
}

void register_mutable(const std::string& name) {
    state.variables[name] = {state.variables[name].type, true, state.variables[name].borrow_count};
}

void register_borrow(const std::string& name) {
    state.variables[name].borrow_count++;
}

bool is_mutable(const std::string& name) {
    auto it = state.variables.find(name);
    return it != state.variables.end() && it->second.is_mutable;
}

int borrow_count(const std::string& name) {
    auto it = state.variables.find(name);
    return it != state.variables.end() ? it->second.borrow_count : 0;
}

void validate_ast(ast::Node* node) {
    validate_statement(node);
}

}
