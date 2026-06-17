#pragma once
#include "../ast/ast.h"
#include "../lexer/token.h"
#include <vector>
#include <memory>
#include <string>

namespace parser {

class Parser {
public:
    Parser(const std::vector<lexer::Token>& tokens);
    std::unique_ptr<ast::ProgramNode> parse();

private:
    const std::vector<lexer::Token>& tokens_;
    size_t pos_;
    bool has_primordial_regalia_ = false;

    lexer::Token peek() const;
    lexer::Token prev() const;
    lexer::Token advance();
    bool match(lexer::TokenType type);
    bool check(lexer::TokenType type) const;

    std::unique_ptr<ast::Node> genesis_declaration();
    std::unique_ptr<ast::Node> genesis_statement();
    std::unique_ptr<ast::Node> genesis_block_body();

    std::unique_ptr<ast::Node> expression();
    std::unique_ptr<ast::Node> assignment();
    std::unique_ptr<ast::Node> equality();
    std::unique_ptr<ast::Node> comparison();
    std::unique_ptr<ast::Node> term();
    std::unique_ptr<ast::Node> factor();
    std::unique_ptr<ast::Node> unary();
    std::unique_ptr<ast::Node> primary();

    void check_permission(const std::string& feature);

    std::unique_ptr<ast::Node> parse_incorporate();
    std::unique_ptr<ast::Node> parse_sanction();
};

std::unique_ptr<ast::ProgramNode> parse_source(const std::string& src);
void validate_sanctions(ast::ProgramNode* program);

}