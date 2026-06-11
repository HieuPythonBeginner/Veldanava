#include "parser.h"
#include "../lexer/lexer.h"
#include <stdexcept>

namespace parser {

Parser::Parser(const std::vector<lexer::Token>& tokens)
    : tokens_(tokens), pos_(0), has_primordial_regalia_(false) {}

lexer::Token Parser::peek() const {
    if (pos_ < tokens_.size()) return tokens_[pos_];
    return lexer::Token(lexer::TokenType::Eof, "");
}

lexer::Token Parser::prev() const {
    if (pos_ > 0) return tokens_[pos_ - 1];
    return lexer::Token(lexer::TokenType::Eof, "");
}

lexer::Token Parser::advance() {
    return tokens_[pos_++];
}

bool Parser::match(lexer::TokenType type) {
    if (check(type)) { pos_++; return true; }
    return false;
}

bool Parser::check(lexer::TokenType type) const {
    return peek().type == type;
}

void Parser::check_permission(const std::string& feature) {
    if (!has_primordial_regalia_) {
        throw std::runtime_error("Permission denied: '" + feature + "' requires Primordial_Regalia at the start of the file");
    }
}

std::unique_ptr<ast::ProgramNode> Parser::parse() {
    auto program = std::make_unique<ast::ProgramNode>();

    if (!check(lexer::TokenType::Primordial_Regalia)) {
        throw std::runtime_error("Missing Primordial_Regalia at the start of the file");
    }
    advance();
    if (!match(lexer::TokenType::Semi)) {
        throw std::runtime_error("Expected ';' after Primordial_Regalia");
    }
    has_primordial_regalia_ = true;

    while (!check(lexer::TokenType::Eof)) {
        if (!match(lexer::TokenType::Genesis)) {
            throw std::runtime_error("Permission denied: top-level declarations require 'genesis' prefix");
        }
        check_permission("genesis");
        auto stmt = genesis_declaration();
        if (stmt) program->statements.push_back(std::move(stmt));
    }
    return program;
}

std::unique_ptr<ast::Node> Parser::genesis_declaration() {
    if (match(lexer::TokenType::Func)) {
        auto func = std::make_unique<ast::GenesisDeclNode>();
        func->kind = "func";
        if (!check(lexer::TokenType::Ident)) throw std::runtime_error("Expected function name after genesis func");
        func->name = advance().lexeme;
        if (check(lexer::TokenType::LParen)) {
            advance();
            while (!check(lexer::TokenType::RParen)) {
                if (!check(lexer::TokenType::Ident)) throw std::runtime_error("Expected param name");
                std::string name = advance().lexeme;
                if (!match(lexer::TokenType::Colon)) throw std::runtime_error("Expected ':'");
                if (check(lexer::TokenType::I32) || check(lexer::TokenType::I64) || check(lexer::TokenType::U32) ||
                    check(lexer::TokenType::U64) || check(lexer::TokenType::F32) || check(lexer::TokenType::F64) ||
                    check(lexer::TokenType::Bool) || check(lexer::TokenType::String) || check(lexer::TokenType::Char) ||
                    check(lexer::TokenType::Void)) {
                    std::string type = advance().lexeme;
                    func->params.push_back({name, type});
                } else {
                    func->params.push_back({name, "i32"});
                }
                if (match(lexer::TokenType::Comma)) continue;
                break;
            }
            if (!match(lexer::TokenType::RParen)) throw std::runtime_error("Expected ')'");
        }
        if (match(lexer::TokenType::RArrow)) {
            if (!check(lexer::TokenType::Ident) && !check(lexer::TokenType::I32) && !check(lexer::TokenType::I64) &&
                !check(lexer::TokenType::U32) && !check(lexer::TokenType::U64) && !check(lexer::TokenType::F32) &&
                !check(lexer::TokenType::F64) && !check(lexer::TokenType::Bool) && !check(lexer::TokenType::String) &&
                !check(lexer::TokenType::Char) && !check(lexer::TokenType::Void)) throw std::runtime_error("Expected return type");
            func->return_type = advance().lexeme;
        }
        if (match(lexer::TokenType::Colon)) {
            if (!check(lexer::TokenType::Indent)) {
                throw std::runtime_error("Expected indent after ':'");
            }
            advance();
            func->body = genesis_block_body();
        } else {
            throw std::runtime_error("Expected ':' after genesis func parameters");
        }
        return func;
    }

    if (match(lexer::TokenType::Class)) {
        auto func = std::make_unique<ast::GenesisDeclNode>();
        func->kind = "class";
        if (!check(lexer::TokenType::Ident)) throw std::runtime_error("Expected class name after genesis class");
        func->name = advance().lexeme;
        if (match(lexer::TokenType::Colon)) {
            if (!check(lexer::TokenType::Indent)) {
                throw std::runtime_error("Expected indent after ':'");
            }
            advance();
            func->body = genesis_block_body();
        } else {
            throw std::runtime_error("Expected ':' after genesis class");
        }
        return func;
    }

    if (match(lexer::TokenType::Struct)) {
        auto func = std::make_unique<ast::GenesisDeclNode>();
        func->kind = "struct";
        if (!check(lexer::TokenType::Ident)) throw std::runtime_error("Expected struct name after genesis struct");
        func->name = advance().lexeme;
        if (match(lexer::TokenType::Colon)) {
            if (!check(lexer::TokenType::Indent)) {
                throw std::runtime_error("Expected indent after ':'");
            }
            advance();
            func->body = genesis_block_body();
        } else {
            throw std::runtime_error("Expected ':' after genesis struct");
        }
        return func;
    }

    if (match(lexer::TokenType::Var)) {
        auto func = std::make_unique<ast::GenesisDeclNode>();
        func->kind = "var";
        if (!check(lexer::TokenType::Ident)) throw std::runtime_error("Expected variable name after genesis var");
        func->name = advance().lexeme;
        if (check(lexer::TokenType::I32) || check(lexer::TokenType::I64) || check(lexer::TokenType::U32) ||
            check(lexer::TokenType::U64) || check(lexer::TokenType::F32) || check(lexer::TokenType::F64) ||
            check(lexer::TokenType::Bool) || check(lexer::TokenType::String) || check(lexer::TokenType::Char) ||
            check(lexer::TokenType::Void)) {
            func->return_type = advance().lexeme;
        }
        if (match(lexer::TokenType::Assign)) {
            func->body = expression();
        }
        if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';' after genesis var");
        return func;
    }

    if (match(lexer::TokenType::Incorporate)) {
        return parse_incorporate();
    }

    throw std::runtime_error("Expected func, class, struct, var, or Incorporate after genesis");
}

std::unique_ptr<ast::Node> Parser::parse_incorporate() {
    check_permission("Incorporate");
    auto inc = std::make_unique<ast::IncorporateNode>();
    do {
        if (check(lexer::TokenType::StringLit)) {
            inc->modules.push_back(advance().lexeme);
        } else if (check(lexer::TokenType::Ident)) {
            inc->modules.push_back(advance().lexeme);
        } else {
            throw std::runtime_error("Expected module name (identifier or string) after Incorporate");
        }
    } while (match(lexer::TokenType::Comma));
    if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';' after Incorporate");
    return inc;
}

std::unique_ptr<ast::Node> Parser::parse_sanction() {
    check_permission("Sanction");
    auto san = std::make_unique<ast::SanctionBlockNode>();
    if (!match(lexer::TokenType::Colon)) throw std::runtime_error("Expected ':' after Sanction");
    if (!check(lexer::TokenType::Indent)) throw std::runtime_error("Expected indent after ':'");
    advance();
    while (!check(lexer::TokenType::Dedent) && !check(lexer::TokenType::Eof)) {
        if (check(lexer::TokenType::Ident)) {
            san->operators.push_back(advance().lexeme);
        } else {
            throw std::runtime_error("Expected operator identifier in Sanction block");
        }
        if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';' after operator in Sanction");
    }
    if (check(lexer::TokenType::Dedent)) {
        match(lexer::TokenType::Dedent);
    }
    if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';' after Sanction block");
    return san;
}

std::unique_ptr<ast::Node> Parser::genesis_statement() {
    if (match(lexer::TokenType::Sanction)) {
        return parse_sanction();
    }
    if (match(lexer::TokenType::Let) || match(lexer::TokenType::Const)) {
        auto decl = std::make_unique<ast::VarDeclNode>();
        if (check(lexer::TokenType::I32) || check(lexer::TokenType::I64) || check(lexer::TokenType::U32) ||
            check(lexer::TokenType::U64) || check(lexer::TokenType::F32) || check(lexer::TokenType::F64) ||
            check(lexer::TokenType::Bool) || check(lexer::TokenType::String) || check(lexer::TokenType::Char) ||
            check(lexer::TokenType::Void)) {
            decl->type = advance().lexeme;
            if (!check(lexer::TokenType::Ident)) throw std::runtime_error("Expected identifier after type");
            decl->name = advance().lexeme;
        } else {
            throw std::runtime_error("Expected type after let/const");
        }
        decl->is_mutable = true;
        if (match(lexer::TokenType::Assign)) {
            decl->initializer = expression();
        }
        if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';'");
        return decl;
    }
    if (match(lexer::TokenType::If)) {
        auto ifstmt = std::make_unique<ast::IfStmtNode>();
        if (check(lexer::TokenType::LParen)) {
            advance();
            ifstmt->condition = expression();
            if (!match(lexer::TokenType::RParen)) throw std::runtime_error("Expected ')'");
        } else {
            ifstmt->condition = expression();
        }
        if (!match(lexer::TokenType::Colon)) throw std::runtime_error("Expected ':'");
        if (!check(lexer::TokenType::Indent)) throw std::runtime_error("Expected indent after ':'");
        advance();
        ifstmt->then_block = genesis_block_body();
        if (match(lexer::TokenType::Else)) {
            if (!match(lexer::TokenType::Colon)) throw std::runtime_error("Expected ':'");
            if (!check(lexer::TokenType::Indent)) throw std::runtime_error("Expected indent after ':'");
            advance();
            ifstmt->else_block = genesis_block_body();
        }
        return ifstmt;
    }
    if (match(lexer::TokenType::While)) {
        auto whilestmt = std::make_unique<ast::WhileStmtNode>();
        if (check(lexer::TokenType::LParen)) {
            advance();
            whilestmt->condition = expression();
            if (!match(lexer::TokenType::RParen)) throw std::runtime_error("Expected ')'");
        } else {
            whilestmt->condition = expression();
        }
        if (!match(lexer::TokenType::Colon)) throw std::runtime_error("Expected ':'");
        if (!check(lexer::TokenType::Indent)) throw std::runtime_error("Expected indent after ':'");
        advance();
        whilestmt->body = genesis_block_body();
        return whilestmt;
    }
    if (match(lexer::TokenType::For)) {
        auto forstmt = std::make_unique<ast::ForStmtNode>();
        if (!check(lexer::TokenType::Ident)) throw std::runtime_error("Expected iterator");
        forstmt->iterator = advance().lexeme;
        if (match(lexer::TokenType::In)) {
            forstmt->bound = expression();
        }
        if (!match(lexer::TokenType::Colon)) throw std::runtime_error("Expected ':'");
        if (!check(lexer::TokenType::Indent)) throw std::runtime_error("Expected indent after ':'");
        advance();
        forstmt->body = genesis_block_body();
        return forstmt;
    }
    if (match(lexer::TokenType::Return)) {
        auto ret = std::make_unique<ast::ReturnStmtNode>();
        ret->value = expression();
        if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';'");
        return ret;
    }
    if (match(lexer::TokenType::Break)) {
        if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';'");
        return std::make_unique<ast::BreakStmtNode>();
    }
    if (match(lexer::TokenType::Continue)) {
        if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';'");
        return std::make_unique<ast::ContinueStmtNode>();
    }
    if (match(lexer::TokenType::Func)) {
        return genesis_declaration();
    }
    if (match(lexer::TokenType::Class)) {
        return genesis_declaration();
    }
    if (match(lexer::TokenType::Struct)) {
        return genesis_declaration();
    }
    if (match(lexer::TokenType::Var)) {
        return genesis_declaration();
    }
    if (match(lexer::TokenType::Incorporate)) {
        return parse_incorporate();
    }
    if (check(lexer::TokenType::Ident)) {
        size_t save_pos = pos_;
        advance();
        if (check(lexer::TokenType::LParen)) {
            pos_ = save_pos;
            auto call = std::make_unique<ast::FuncCallNode>();
            call->name = advance().lexeme;
            if (!match(lexer::TokenType::LParen)) throw std::runtime_error("Expected '('");
            while (!check(lexer::TokenType::RParen) && !check(lexer::TokenType::Eof)) {
                call->args.push_back(expression());
                if (match(lexer::TokenType::Comma)) continue;
                break;
            }
            if (!match(lexer::TokenType::RParen)) throw std::runtime_error("Expected ')'");
            if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';'");
            return call;
        }
        pos_ = save_pos;
    }
    throw std::runtime_error("Unexpected token after genesis: " + peek().lexeme);
}

std::unique_ptr<ast::Node> Parser::genesis_block_body() {
    auto block = std::make_unique<ast::BlockNode>();
    while (!check(lexer::TokenType::Dedent) && !check(lexer::TokenType::Eof)) {
        if (!match(lexer::TokenType::Genesis)) {
            throw std::runtime_error("Expected 'genesis' prefix for all statements in genesis scope");
        }
        auto stmt = genesis_statement();
        if (stmt) block->statements.push_back(std::move(stmt));
    }
    while (check(lexer::TokenType::Dedent)) {
        match(lexer::TokenType::Dedent);
    }
    if (!check(lexer::TokenType::Semi)) {
        throw std::runtime_error("Expected ';' after dedent to close block");
    }
    match(lexer::TokenType::Semi);
    return block;
}

std::unique_ptr<ast::Node> Parser::expression() { return assignment(); }
std::unique_ptr<ast::Node> Parser::assignment() { return equality(); }
std::unique_ptr<ast::Node> Parser::equality() {
    auto left = comparison();
    while (match(lexer::TokenType::And) || match(lexer::TokenType::Or)) {
        auto bin = std::make_unique<ast::BinaryExprNode>();
        bin->op = prev().type == lexer::TokenType::And ? ast::BinaryOp::And : ast::BinaryOp::Or;
        bin->left = std::move(left);
        bin->right = comparison();
        left = std::move(bin);
    }
    return left;
}
std::unique_ptr<ast::Node> Parser::comparison() {
    auto left = term();
    while (match(lexer::TokenType::Lt) || match(lexer::TokenType::Gt) || match(lexer::TokenType::Le) ||
           match(lexer::TokenType::Ge) || match(lexer::TokenType::Eq) || match(lexer::TokenType::Ne)) {
        auto bin = std::make_unique<ast::BinaryExprNode>();
        lexer::TokenType t = prev().type;
        if (t == lexer::TokenType::Lt) bin->op = ast::BinaryOp::Lt;
        else if (t == lexer::TokenType::Gt) bin->op = ast::BinaryOp::Gt;
        else if (t == lexer::TokenType::Le) bin->op = ast::BinaryOp::Le;
        else if (t == lexer::TokenType::Ge) bin->op = ast::BinaryOp::Ge;
        else if (t == lexer::TokenType::Eq) bin->op = ast::BinaryOp::Eq;
        else bin->op = ast::BinaryOp::Ne;
        bin->left = std::move(left);
        bin->right = term();
        left = std::move(bin);
    }
    return left;
}

std::unique_ptr<ast::Node> Parser::term() {
    auto left = factor();
    while (match(lexer::TokenType::Add) || match(lexer::TokenType::Sub)) {
        auto bin = std::make_unique<ast::BinaryExprNode>();
        bin->op = prev().type == lexer::TokenType::Add ? ast::BinaryOp::Add : ast::BinaryOp::Sub;
        bin->left = std::move(left);
        bin->right = factor();
        left = std::move(bin);
    }
    return left;
}

std::unique_ptr<ast::Node> Parser::factor() {
    auto left = unary();
    while (match(lexer::TokenType::Mul) || match(lexer::TokenType::Div) || match(lexer::TokenType::Mod)) {
        auto bin = std::make_unique<ast::BinaryExprNode>();
        if (prev().type == lexer::TokenType::Mul) bin->op = ast::BinaryOp::Mul;
        else if (prev().type == lexer::TokenType::Div) bin->op = ast::BinaryOp::Div;
        else bin->op = ast::BinaryOp::Mod;
        bin->left = std::move(left);
        bin->right = unary();
        left = std::move(bin);
    }
    return left;
}

std::unique_ptr<ast::Node> Parser::unary() {
    if (match(lexer::TokenType::Sub) || match(lexer::TokenType::Not)) {
        auto un = std::make_unique<ast::UnaryExprNode>();
        un->op = prev().type == lexer::TokenType::Sub ? ast::UnaryOp::Neg : ast::UnaryOp::Not;
        un->operand = unary();
        return un;
    }
    return primary();
}

std::unique_ptr<ast::Node> Parser::primary() {
    if (match(lexer::TokenType::IntLit)) {
        auto lit = std::make_unique<ast::LiteralNode>();
        lit->kind = ast::LiteralKind::Int;
        lit->value = prev().lexeme;
        return lit;
    }
    if (match(lexer::TokenType::FloatLit)) {
        auto lit = std::make_unique<ast::LiteralNode>();
        lit->kind = ast::LiteralKind::Float;
        lit->value = prev().lexeme;
        return lit;
    }
    if (match(lexer::TokenType::BoolLit)) {
        auto lit = std::make_unique<ast::LiteralNode>();
        lit->kind = ast::LiteralKind::Bool;
        lit->value = prev().lexeme;
        return lit;
    }
    if (match(lexer::TokenType::StringLit)) {
        auto lit = std::make_unique<ast::LiteralNode>();
        lit->kind = ast::LiteralKind::String;
        lit->value = prev().lexeme;
        return lit;
    }
    if (check(lexer::TokenType::Ident)) {
        lexer::Token next = pos_ + 1 < tokens_.size() ? tokens_[pos_ + 1] : lexer::Token(lexer::TokenType::Eof, "");
        if (next.type == lexer::TokenType::LParen) {
            auto call = std::make_unique<ast::FuncCallNode>();
            call->name = advance().lexeme;
            if (!match(lexer::TokenType::LParen)) throw std::runtime_error("Expected '('");
            while (!check(lexer::TokenType::RParen) && !check(lexer::TokenType::Eof)) {
                call->args.push_back(expression());
                if (match(lexer::TokenType::Comma)) continue;
                break;
            }
            if (!match(lexer::TokenType::RParen)) throw std::runtime_error("Expected ')'");
            return call;
        }
        if (match(lexer::TokenType::Ident)) {
            auto id = std::make_unique<ast::IdentifierNode>();
            id->name = prev().lexeme;
            return id;
        }
    }
    throw std::runtime_error("Unexpected token: " + peek().lexeme);
}

std::unique_ptr<ast::ProgramNode> parse_source(const std::string& src) {
    auto tokens = lexer::tokenize(src);
    Parser parser(tokens);
    return parser.parse();
}

}
