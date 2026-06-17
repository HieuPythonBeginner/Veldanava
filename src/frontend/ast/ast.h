#pragma once
#include <string>
#include <vector>
#include <memory>

namespace ast {

enum class NodeKind {
    Program,
    Block,
    VarDecl,
    ConstDecl,
    Assign,
    BinaryExpr,
    UnaryExpr,
    Identifier,
    Literal,
    IfStmt,
    WhileStmt,
    ForStmt,
    ReturnStmt,
    FuncDef,
    FuncCall,
    BreakStmt,
    ContinueStmt,
    GenesisDecl,
    IncorporateStmt,
    SanctionBlock,
};



enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Lt, Gt, Le, Ge, Eq, Ne,
    And, Or,
};

enum class UnaryOp {
    Neg, Not,
};

enum class LiteralKind {
    Int, Float, Bool, String, Char,
};

struct OwnershipInfo {
    bool is_mutable = false;
    bool is_borrowed = false;
    int scope_level = 0;
};

struct Node {
    NodeKind kind;
    int line = 0;
    int col = 0;
    OwnershipInfo ownership;

    Node(NodeKind k) : kind(k) {}
    virtual ~Node() = default;
};

struct ProgramNode : Node {
    std::vector<std::unique_ptr<Node>> statements;
    ProgramNode() : Node(NodeKind::Program) {}
};

struct BlockNode : Node {
    std::vector<std::unique_ptr<Node>> statements;
    BlockNode() : Node(NodeKind::Block) {}
};

struct VarDeclNode : Node {
    std::string name;
    std::string type;
    std::unique_ptr<Node> initializer;
    bool is_mutable = false;
    VarDeclNode() : Node(NodeKind::VarDecl) {}
};

struct BinaryExprNode : Node {
    BinaryOp op;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    BinaryExprNode() : Node(NodeKind::BinaryExpr) {}
};

struct UnaryExprNode : Node {
    UnaryOp op;
    std::unique_ptr<Node> operand;
    UnaryExprNode() : Node(NodeKind::UnaryExpr) {}
};

struct IdentifierNode : Node {
    std::string name;
    IdentifierNode() : Node(NodeKind::Identifier) {}
};

struct LiteralNode : Node {
    LiteralKind kind;
    std::string value;
    LiteralNode() : Node(NodeKind::Literal) {}
};

struct IfStmtNode : Node {
    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> then_block;
    std::unique_ptr<Node> else_block;
    std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>> elif_chain;
    IfStmtNode() : Node(NodeKind::IfStmt) {}
};

struct WhileStmtNode : Node {
    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> body;
    WhileStmtNode() : Node(NodeKind::WhileStmt) {}
};

struct ForStmtNode : Node {
    std::string iterator;
    std::unique_ptr<Node> bound;
    std::unique_ptr<Node> body;
    ForStmtNode() : Node(NodeKind::ForStmt) {}
};

struct ReturnStmtNode : Node {
    std::unique_ptr<Node> value;
    ReturnStmtNode() : Node(NodeKind::ReturnStmt) {}
};

struct FuncDefNode : Node {
    std::string name;
    std::vector<std::pair<std::string, std::string>> params;
    std::string return_type;
    std::unique_ptr<Node> body;
    FuncDefNode() : Node(NodeKind::FuncDef) {}
};

struct FuncCallNode : Node {
    std::string name;
    std::vector<std::unique_ptr<Node>> args;
    FuncCallNode() : Node(NodeKind::FuncCall) {}
};

struct BreakStmtNode : Node {
    BreakStmtNode() : Node(NodeKind::BreakStmt) {}
};

struct ContinueStmtNode : Node {
    ContinueStmtNode() : Node(NodeKind::ContinueStmt) {}
};

struct GenesisDeclNode : Node {
    std::string kind;
    std::string name;
    std::vector<std::pair<std::string, std::string>> params;
    std::string return_type;
    std::unique_ptr<Node> body;
    GenesisDeclNode() : Node(NodeKind::GenesisDecl) {}
};

struct IncorporateNode : Node {
    std::vector<std::string> modules;
    IncorporateNode() : Node(NodeKind::IncorporateStmt) {}
};

struct SanctionBlockNode : Node {
    std::vector<std::string> operators;
    std::vector<std::string> funcs;
    std::vector<std::string> oop;
    SanctionBlockNode() : Node(NodeKind::SanctionBlock) {}
};

}
