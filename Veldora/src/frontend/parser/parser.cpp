#include "parser.h"
#include <stdexcept>

namespace veldora {

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

Token Parser::peek() const {
    if (pos_ < tokens_.size()) return tokens_[pos_];
    return tokens_.back();
}

Token Parser::advance() {
    return tokens_[pos_++];
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { pos_++; return true; }
    return false;
}

bool Parser::expect(TokenType type, const std::string& err) {
    if (check(type)) { pos_++; return true; }
    throw std::runtime_error(err + " at " + std::to_string(peek().line) + ":" + std::to_string(peek().col));
}

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program = std::make_unique<ProgramNode>();
    while (!check(TokenType::Eof)) {
        if (check(TokenType::Newline)) { advance(); continue; }
        program->statements.push_back(statement());
        if (check(TokenType::Newline)) advance();
    }
    return program;
}

std::unique_ptr<BlockNode> Parser::parse_block() {
    auto block = std::make_unique<BlockNode>();
    block->line = peek().line;
    block->col = peek().col;
    while (!check(TokenType::End) && !check(TokenType::Else) && !check(TokenType::Eof)) {
        if (check(TokenType::Newline)) { advance(); continue; }
        block->statements.push_back(statement());
        if (check(TokenType::Newline)) advance();
    }
    return block;
}

std::unique_ptr<StmtNode> Parser::statement() {
    if (match(TokenType::Let)) {
        if (!check(TokenType::Ident)) throw std::runtime_error("Expected variable name after 'let'");
        Token name = advance();
        if (!match(TokenType::Equals)) expect(TokenType::Be, "Expected 'equals' or 'be' in let statement");
        auto value = expression();
        auto stmt = std::make_unique<LetStmtNode>(name.line, name.col);
        stmt->name = name.lexeme;
        stmt->value = std::move(value);
        return stmt;
    }

    if (match(TokenType::Say)) {
        auto val = expression();
        auto stmt = std::make_unique<SayStmtNode>(peek().line, peek().col);
        stmt->value = std::move(val);
        return stmt;
    }

    if (match(TokenType::If)) {
        auto stmt = std::make_unique<IfStmtNode>(peek().line, peek().col);
        stmt->condition = parse_equality();
        expect(TokenType::Then, "Expected 'then' after condition");
        stmt->then_block = parse_block();
        if (match(TokenType::Else)) {
            stmt->else_block = parse_block();
        }
        expect(TokenType::End, "Expected 'end' to close if block");
        return stmt;
    }

    if (match(TokenType::While)) {
        auto stmt = std::make_unique<WhileStmtNode>(peek().line, peek().col);
        stmt->condition = parse_equality();
        expect(TokenType::Then, "Expected 'then' after while condition");
        stmt->body = parse_block();
        expect(TokenType::End, "Expected 'end' to close while block");
        return stmt;
    }

    if (match(TokenType::Func)) {
        if (!check(TokenType::Ident)) throw std::runtime_error("Expected function name after 'func'");
        Token name = advance();
        auto decl = std::make_unique<FuncDeclNode>(name.line, name.col);
        decl->name = name.lexeme;
        if (match(TokenType::LeftParen)) {
            while (!check(TokenType::RightParen) && !check(TokenType::Eof)) {
                if (!check(TokenType::Ident)) throw std::runtime_error("Expected parameter name");
                decl->params.push_back(advance().lexeme);
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RightParen, "Expected ')' after parameters");
        }
        decl->body = parse_block();
        expect(TokenType::End, "Expected 'end' to close func block");
        return decl;
    }

    if (match(TokenType::Return)) {
        auto stmt = std::make_unique<ReturnStmtNode>(peek().line, peek().col);
        stmt->value = expression();
        return stmt;
    }

    auto expr = expression();
    return std::make_unique<ExprStmtNode>(expr->line, expr->col);
}

std::vector<std::unique_ptr<ExprNode>> Parser::parse_args() {
    std::vector<std::unique_ptr<ExprNode>> args;
    if (check(TokenType::RightParen)) return args;
    args.push_back(expression());
    while (match(TokenType::Comma)) {
        if (check(TokenType::RightParen)) break;
        args.push_back(expression());
    }
    return args;
}

std::unique_ptr<ExprNode> Parser::expression() { return parse_equality(); }

std::unique_ptr<ExprNode> Parser::parse_equality() {
    auto left = parse_comparison();
    while (check(TokenType::Equals) || check(TokenType::NotEquals)) {
        Token op = advance();
        auto right = parse_comparison();
        auto node = std::make_unique<BinaryExprNode>(op.lexeme, op.line, op.col);
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_comparison() {
    auto left = parse_term();
    while (check(TokenType::LessThan) || check(TokenType::GreaterThan) ||
           check(TokenType::LessOrEqual) || check(TokenType::GreaterOrEqual)) {
        Token op = advance();
        auto right = parse_term();
        auto node = std::make_unique<BinaryExprNode>(op.lexeme, op.line, op.col);
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_term() {
    auto left = parse_factor();
    while (check(TokenType::Add) || check(TokenType::Subtract)) {
        Token op = advance();
        auto right = parse_factor();
        auto node = std::make_unique<BinaryExprNode>(op.lexeme, op.line, op.col);
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_factor() {
    auto left = parse_unary();
    while (check(TokenType::Multiply) || check(TokenType::Divide)) {
        Token op = advance();
        auto right = parse_unary();
        auto node = std::make_unique<BinaryExprNode>(op.lexeme, op.line, op.col);
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_unary() {
    if (check(TokenType::Add) || check(TokenType::Subtract) || check(TokenType::Not)) {
        Token op = advance();
        auto operand = parse_unary();
        auto node = std::make_unique<UnaryExprNode>(op.lexeme, op.line, op.col);
        node->operand = std::move(operand);
        return node;
    }
    return primary();
}

std::unique_ptr<ExprNode> Parser::primary() {
    if (match(TokenType::Number)) {
        Token t = tokens_[pos_ - 1];
        return std::make_unique<NumberExprNode>(std::stoi(t.lexeme), t.line, t.col);
    }
    if (match(TokenType::String)) {
        Token t = tokens_[pos_ - 1];
        return std::make_unique<StringExprNode>(t.lexeme, t.line, t.col);
    }
    if (match(TokenType::Ident)) {
        Token name = tokens_[pos_ - 1];
        auto node = std::make_unique<IdentExprNode>(name.lexeme, name.line, name.col);
        if (match(TokenType::LeftParen)) {
            auto call = std::make_unique<CallExprNode>(name.lexeme, name.line, name.col);
            call->args = parse_args();
            expect(TokenType::RightParen, "Expected ')' after call args");
            return call;
        }
        return node;
    }
    if (match(TokenType::LeftParen)) {
        auto expr = expression();
        expect(TokenType::RightParen, "Expected ')' after expression");
        return expr;
    }
    Token t = peek();
    throw std::runtime_error("Unexpected token in expression: " + peek().lexeme);
}

std::unique_ptr<ProgramNode> parse_source(const std::string& src) {
    auto tokens = tokenize(src);
    return parse_tokens(tokens);
}

std::unique_ptr<ProgramNode> parse_tokens(const std::vector<Token>& tokens) {
    Parser parser(tokens);
    return parser.parse();
}

}
