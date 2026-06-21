#pragma once
#include <string>
#include <vector>
#include <memory>

namespace ast {
    enum class NodeKind {
        Program, Block, VarDecl, Literal, ExprStmt
    };
    struct Node {
        NodeKind kind;
        int line=0, col=0;
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
        VarDeclNode() : Node(NodeKind::VarDecl) {}
    };
    struct LiteralNode : Node {
        std::string value;
        LiteralNode() : Node(NodeKind::Literal) {}
    };
    struct ExprStmtNode : Node {
        std::unique_ptr<Node> expression;
        ExprStmtNode() : Node(NodeKind::ExprStmt) {}
    };
}
