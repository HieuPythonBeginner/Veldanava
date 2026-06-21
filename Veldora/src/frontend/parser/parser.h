#pragma once
#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include <vector>
#include <memory>
#include <string>

namespace veldora {

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ProgramNode> parse();

private:
    const std::vector<Token>& tokens_;
    size_t pos_;

    Token peek() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool expect(TokenType type, const std::string& err);

    std::unique_ptr<StmtNode> statement();
    std::unique_ptr<ExprNode> expression();
    std::unique_ptr<ExprNode> primary();
    std::unique_ptr<ExprNode> parse_equality();
    std::unique_ptr<ExprNode> parse_comparison();
    std::unique_ptr<ExprNode> parse_term();
    std::unique_ptr<ExprNode> parse_factor();
    std::unique_ptr<ExprNode> parse_unary();
    std::unique_ptr<BlockNode> parse_block();
    std::vector<std::unique_ptr<ExprNode>> parse_args();
};

std::unique_ptr<ProgramNode> parse_source(const std::string& src);
std::unique_ptr<ProgramNode> parse_tokens(const std::vector<Token>& tokens);

}
