#include "ownership.h"
#include "ast/ast.h"
#include <unordered_map>
#include <string>

namespace ownership {

struct OwnershipState {
    std::unordered_map<std::string, bool> mutable_vars;
    std::unordered_map<std::string, int> borrow_count;
    int current_scope = 0;
};

static OwnershipState state;

void init() {
    state.mutable_vars.clear();
    state.borrow_count.clear();
    state.current_scope = 0;
}

void enter_scope() {
    state.current_scope++;
}

void exit_scope() {
    state.current_scope--;
}

void register_mutable(const std::string& name) {
    state.mutable_vars[name] = true;
}

void register_borrow(const std::string& name) {
    state.borrow_count[name]++;
}

bool is_mutable(const std::string& name) {
    auto it = state.mutable_vars.find(name);
    return it != state.mutable_vars.end() && it->second;
}

int borrow_count(const std::string& name) {
    auto it = state.borrow_count.find(name);
    return it != state.borrow_count.end() ? it->second : 0;
}

void validate_ast(ast::Node* node) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::VarDecl: {
            auto* decl = static_cast<ast::VarDeclNode*>(node);
            if (decl->is_mutable) {
                register_mutable(decl->name);
            }
            break;
        }
        case ast::NodeKind::Block: {
            enter_scope();
            auto* block = static_cast<ast::BlockNode*>(node);
            for (auto& stmt : block->statements) {
                validate_ast(stmt.get());
            }
            exit_scope();
            break;
        }
        case ast::NodeKind::IfStmt: {
            auto* stmt = static_cast<ast::IfStmtNode*>(node);
            validate_ast(stmt->condition.get());
            enter_scope();
            validate_ast(stmt->then_block.get());
            exit_scope();
            if (stmt->else_block) {
                enter_scope();
                validate_ast(stmt->else_block.get());
                exit_scope();
            }
            for (auto& [cond, body] : stmt->elif_chain) {
                validate_ast(cond.get());
                enter_scope();
                validate_ast(body.get());
                exit_scope();
            }
            break;
        }
        case ast::NodeKind::WhileStmt: {
            auto* stmt = static_cast<ast::WhileStmtNode*>(node);
            validate_ast(stmt->condition.get());
            enter_scope();
            validate_ast(stmt->body.get());
            exit_scope();
            break;
        }
        case ast::NodeKind::ForStmt: {
            auto* stmt = static_cast<ast::ForStmtNode*>(node);
            validate_ast(stmt->bound.get());
            enter_scope();
            validate_ast(stmt->body.get());
            exit_scope();
            break;
        }
        case ast::NodeKind::FuncDef:
        case ast::NodeKind::GenesisDecl: {
            auto* decl = static_cast<ast::GenesisDeclNode*>(node);
            enter_scope();
            if (decl->body) validate_ast(decl->body.get());
            exit_scope();
            break;
        }
        default:
            break;
    }
}

}
