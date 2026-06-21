#pragma once
#include <vector>
#include <memory>
#include <string>

namespace veldora {

struct Token;

enum class NodeKind {
    Program,
    LetStmt,
    SayStmt,
    IfStmt,
    WhileStmt,
    FuncDecl,
    ReturnStmt,
    Expr,
    ExprStmt,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    IdentExpr,
    NumberExpr,
    StringExpr,
    Block,
};

struct Node {
    NodeKind kind;
    int line;
    int col;
    Node(NodeKind k, int ln, int c) : kind(k), line(ln), col(c) {}
    virtual ~Node() = default;
};

struct ExprNode : Node {
    ExprNode(NodeKind k, int ln, int c) : Node(k, ln, c) {}
};

struct StmtNode : Node {
    StmtNode(NodeKind k, int ln, int c) : Node(k, ln, c) {}
};

struct ProgramNode : Node {
    std::vector<std::unique_ptr<StmtNode>> statements;
    ProgramNode() : Node(NodeKind::Program, 1, 1) {}
};

struct BlockNode : Node {
    std::vector<std::unique_ptr<StmtNode>> statements;
    BlockNode() : Node(NodeKind::Block, 0, 0) {}
};

struct LetStmtNode : StmtNode {
    std::string name;
    std::unique_ptr<ExprNode> value;
    LetStmtNode(int ln, int c) : StmtNode(NodeKind::LetStmt, ln, c) {}
};

struct SayStmtNode : StmtNode {
    std::unique_ptr<ExprNode> value;
    SayStmtNode(int ln, int c) : StmtNode(NodeKind::SayStmt, ln, c) {}
};

struct IfStmtNode : StmtNode {
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<BlockNode> then_block;
    std::unique_ptr<BlockNode> else_block;
    IfStmtNode(int ln, int c) : StmtNode(NodeKind::IfStmt, ln, c) {}
};

struct WhileStmtNode : StmtNode {
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<BlockNode> body;
    WhileStmtNode(int ln, int c) : StmtNode(NodeKind::WhileStmt, ln, c) {}
};

struct FuncDeclNode : StmtNode {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockNode> body;
    FuncDeclNode(int ln, int c) : StmtNode(NodeKind::FuncDecl, ln, c) {}
};

struct ExprStmtNode : StmtNode {
    std::unique_ptr<ExprNode> expression;
    ExprStmtNode(int ln, int c) : StmtNode(NodeKind::ExprStmt, ln, c) {}
};

struct ReturnStmtNode : StmtNode {
    std::unique_ptr<ExprNode> value;
    ReturnStmtNode(int ln, int c) : StmtNode(NodeKind::ReturnStmt, ln, c) {}
};

struct BinaryExprNode : ExprNode {
    std::string op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;
    BinaryExprNode(const std::string& o, int ln, int c)
        : ExprNode(NodeKind::BinaryExpr, ln, c), op(o) {}
};

struct UnaryExprNode : ExprNode {
    std::string op;
    std::unique_ptr<ExprNode> operand;
    UnaryExprNode(const std::string& o, int ln, int c)
        : ExprNode(NodeKind::UnaryExpr, ln, c), op(o) {}
};

struct CallExprNode : ExprNode {
    std::string callee;
    std::vector<std::unique_ptr<ExprNode>> args;
    CallExprNode(const std::string& name, int ln, int c)
        : ExprNode(NodeKind::CallExpr, ln, c), callee(name) {}
};

struct IdentExprNode : ExprNode {
    std::string name;
    IdentExprNode(const std::string& n, int ln, int c)
        : ExprNode(NodeKind::IdentExpr, ln, c), name(n) {}
};

struct NumberExprNode : ExprNode {
    int value;
    NumberExprNode(int v, int ln, int c)
        : ExprNode(NodeKind::NumberExpr, ln, c), value(v) {}
};

struct StringExprNode : ExprNode {
    std::string value;
    StringExprNode(const std::string& v, int ln, int c)
        : ExprNode(NodeKind::StringExpr, ln, c), value(v) {}
};

}
