#include "interpreter.h"
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <cstdlib>

namespace veldora {

Interpreter::Interpreter() {
    enter_scope();
}

void Interpreter::enter_scope() {
    scopes_.push_back(std::unordered_map<std::string, int>());
}

void Interpreter::exit_scope() {
    if (scopes_.size() > 1) scopes_.pop_back();
}

int Interpreter::eval_expr(ExprNode* node) {
    switch (node->kind) {
        case NodeKind::NumberExpr: {
            auto* n = static_cast<NumberExprNode*>(node);
            return n->value;
        }
        case NodeKind::StringExpr: {
            return 0;
        }
        case NodeKind::IdentExpr: {
            auto* id = static_cast<IdentExprNode*>(node);
            for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
                if (it->count(id->name)) return (*it)[id->name];
            }
            throw std::runtime_error("Undefined variable: " + id->name);
        }
        case NodeKind::BinaryExpr: {
            auto* b = static_cast<BinaryExprNode*>(node);
            int left = eval_expr(b->left.get());
            int right = eval_expr(b->right.get());
            if (b->op == "add") return left + right;
            if (b->op == "subtract") return left - right;
            if (b->op == "multiply") return left * right;
            if (b->op == "divide") {
                if (right == 0) throw std::runtime_error("Division by zero");
                return left / right;
            }
            if (b->op == "equals") return left == right;
            if (b->op == "not_equals") return left != right;
            if (b->op == "less_than") return left < right;
            if (b->op == "greater_than") return left > right;
            if (b->op == "less_or_equal") return left <= right;
            if (b->op == "greater_or_equal") return left >= right;
            if (b->op == "and") return left && right;
            if (b->op == "or") return left || right;
            throw std::runtime_error("Unknown operator: " + b->op);
        }
        case NodeKind::UnaryExpr: {
            auto* u = static_cast<UnaryExprNode*>(node);
            int val = eval_expr(u->operand.get());
            if (u->op == "subtract") return -val;
            if (u->op == "not") return !val;
            throw std::runtime_error("Unknown unary operator: " + u->op);
        }
        case NodeKind::CallExpr: {
            auto* c = static_cast<CallExprNode*>(node);
            if (c->callee == "print") {
                if (!c->args.empty()) std::cout << eval_expr(c->args[0].get());
                std::cout << "\n";
                return 0;
            }
            if (c->callee == "range") {
                return c->args.empty() ? 0 : eval_expr(c->args[0].get());
            }
            if (c->callee == "sqrt") {
                return c->args.empty() ? 0 : (int)std::sqrt(eval_expr(c->args[0].get()));
            }
            if (c->callee == "sin") {
                return c->args.empty() ? 0 : (int)std::sin(eval_expr(c->args[0].get()));
            }
            if (c->callee == "cos") {
                return c->args.empty() ? 0 : (int)std::cos(eval_expr(c->args[0].get()));
            }
            if (c->callee == "abs") {
                return c->args.empty() ? 0 : std::abs(eval_expr(c->args[0].get()));
            }
            if (c->callee == "floor") {
                return c->args.empty() ? 0 : (int)std::floor(eval_expr(c->args[0].get()));
            }
            if (c->callee == "ceil") {
                return c->args.empty() ? 0 : (int)std::ceil(eval_expr(c->args[0].get()));
            }
            if (c->callee == "pow") {
                if (c->args.size() < 2) return 0;
                return (int)std::pow(eval_expr(c->args[0].get()), eval_expr(c->args[1].get()));
            }
            if (functions_.count(c->callee)) {
                return functions_[c->callee];
            }
            throw std::runtime_error("Unknown function: " + c->callee);
        }
        default:
            throw std::runtime_error("Unknown expression kind");
    }
}

int Interpreter::eval_stmt(StmtNode* node) {
    switch (node->kind) {
        case NodeKind::LetStmt: {
            auto* s = static_cast<LetStmtNode*>(node);
            scopes_.back()[s->name] = eval_expr(s->value.get());
            return 0;
        }
        case NodeKind::SayStmt: {
            auto* s = static_cast<SayStmtNode*>(node);
            if (s->value) {
                if (s->value->kind == NodeKind::StringExpr) {
                    auto* str = static_cast<StringExprNode*>(s->value.get());
                    std::cout << str->value << "\n";
                } else {
                    int val = eval_expr(s->value.get());
                    std::cout << val << "\n";
                }
            }
            return 0;
        }
        case NodeKind::IfStmt: {
            auto* s = static_cast<IfStmtNode*>(node);
            if (eval_expr(s->condition.get())) {
                for (auto& stmt : s->then_block->statements) eval_stmt(stmt.get());
            } else if (s->else_block) {
                for (auto& stmt : s->else_block->statements) eval_stmt(stmt.get());
            }
            return 0;
        }
        case NodeKind::WhileStmt: {
            auto* s = static_cast<WhileStmtNode*>(node);
            enter_scope();
            while (eval_expr(s->condition.get())) {
                for (auto& stmt : s->body->statements) eval_stmt(stmt.get());
            }
            exit_scope();
            return 0;
        }
        case NodeKind::FuncDecl: {
            auto* s = static_cast<FuncDeclNode*>(node);
            functions_[s->name] = 0;
            return 0;
        }
        case NodeKind::ReturnStmt: {
            auto* s = static_cast<ReturnStmtNode*>(node);
            return s->value ? eval_expr(s->value.get()) : 0;
        }
        case NodeKind::ExprStmt: {
            auto* s = static_cast<ExprStmtNode*>(node);
            return eval_expr(s->expression.get());
        }
        default:
            return 0;
    }
}

int Interpreter::eval(ProgramNode* program) {
    for (auto& stmt : program->statements) {
        eval_stmt(stmt.get());
    }
    return 0;
}

}
