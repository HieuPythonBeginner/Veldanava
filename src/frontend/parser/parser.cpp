#include "parser.h"
#include "../lexer/lexer.h"
#include <stdexcept>
#include <iostream>
#include <unordered_set>


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

static bool is_sanction_keyword(const lexer::Token& tok) {
    return tok.type == lexer::TokenType::Sanction || (tok.type == lexer::TokenType::Ident && tok.lexeme == "sanction");
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
        if (match(lexer::TokenType::Semi)) {
            continue;
        }

        // Top-level can be:
        // - Sanction block: `Sanction:` or `sanction:` (lowercase)
        // - genesis <declaration>
        if (is_sanction_keyword(peek())) {
            // Consume whichever token it is (Sanction or Ident)
            if (match(lexer::TokenType::Sanction)) {
                auto san = parse_sanction();
                if (san) program->statements.push_back(std::move(san));
                continue;
            }
            if (match(lexer::TokenType::Ident)) {
                auto san = parse_sanction();
                if (san) program->statements.push_back(std::move(san));
                continue;
            }
            throw std::runtime_error("Internal error: failed to consume sanction keyword");
        }

        if (!check(lexer::TokenType::Genesis)) {
            throw std::runtime_error(
                "Permission denied: top-level declarations require 'genesis' prefix"
            );
        }
        match(lexer::TokenType::Genesis);

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
    check_permission("sanction");
    auto san = std::make_unique<ast::SanctionBlockNode>();

    if (!match(lexer::TokenType::Colon)) {
        if (match(lexer::TokenType::Sanction)) {
            if (!match(lexer::TokenType::Colon)) {
                throw std::runtime_error("Expected ':' after sanction keyword");
            }
        } else {
            throw std::runtime_error("Expected ':' after sanction keyword");
        }
    }

    if (!check(lexer::TokenType::Indent)) throw std::runtime_error("Expected indent after ':'");
    advance();

    std::vector<std::string> operator_names;
    std::vector<std::string> func_names;

    auto add_unique_sanction_entry = [&](const std::string& name, bool is_call) {
        const auto& existing_names = is_call ? func_names : operator_names;
        for (const auto& existing : existing_names) {
            if (existing == name) {
                throw std::runtime_error("Duplicate sanction entry: '" + name + "'");
            }
        }
        if (is_call) {
            func_names.push_back(name);
        } else {
            operator_names.push_back(name);
        }
    };

    auto parse_flat_entries = [&]() {
        while (!check(lexer::TokenType::Dedent) && !check(lexer::TokenType::Semi) && !check(lexer::TokenType::Eof)) {
            if (!check(lexer::TokenType::Ident)) {
                throw std::runtime_error("Expected sanction entry (operator or call)");
            }
            if (pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].type == lexer::TokenType::Colon) {
                throw std::runtime_error("Sanction block does not support operators/funcs/oop sections; use flat entries only");
            }

            std::string name = advance().lexeme;
            if (match(lexer::TokenType::LParen)) {
                if (!match(lexer::TokenType::RParen)) {
                    throw std::runtime_error("Expected ')' in sanction entry");
                }
                add_unique_sanction_entry(name, true);
                san->funcs.push_back(name);
            } else {
                add_unique_sanction_entry(name, false);
                san->operators.push_back(name);
            }
            if (!match(lexer::TokenType::Semi)) {
                throw std::runtime_error("Expected ';' after sanction entry");
            }
        }
    };

    parse_flat_entries();

    while (match(lexer::TokenType::Dedent)) {}
    if (!match(lexer::TokenType::Semi)) throw std::runtime_error("Expected ';' after Sanction block");
    return san;
}


std::unique_ptr<ast::Node> Parser::genesis_statement() {
    if (match(lexer::TokenType::Sanction)) {
        return parse_sanction();
    }
    if (check(lexer::TokenType::Ident) && peek().lexeme == "sanction") {
        advance();
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
            throw std::runtime_error("This language requires: if <cond>: (no parentheses) - got 'if ('");
        }
        ifstmt->condition = expression();
        if (!match(lexer::TokenType::Colon)) throw std::runtime_error("Expected ': '");
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

    if (match(lexer::TokenType::Elif)) {
        throw std::runtime_error("elif is not implemented; use else with nested if");
    }

    if (match(lexer::TokenType::While)) {
        auto whilestmt = std::make_unique<ast::WhileStmtNode>();
        if (check(lexer::TokenType::LParen)) {
            throw std::runtime_error("This language requires: while <cond>: (no parentheses) - got 'while ('");
        }
        whilestmt->condition = expression();
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
        // In genesis scope, statements normally require `genesis` prefix.
        // Allow nested Sanction blocks without requiring the extra `genesis` keyword.
        if (check(lexer::TokenType::Sanction) || (check(lexer::TokenType::Ident) && peek().lexeme == "sanction")) {
            auto stmt = genesis_statement();
            if (stmt) block->statements.push_back(std::move(stmt));
            continue;
        }

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

static std::string operator_sanction_name(ast::BinaryOp op) {
    switch (op) {
        case ast::BinaryOp::Add: return "plus";
        case ast::BinaryOp::Sub: return "minus";
        case ast::BinaryOp::Mul: return "multi";
        case ast::BinaryOp::Div: return "div";
        case ast::BinaryOp::Mod: return "mod";
        default: return "";
    }
}

static bool is_builtin_function(const std::string& name) {
    return name == "print" || name == "range" || name == "scribe";
}

struct SanctionContext {
    std::unordered_set<std::string> operators;
    std::unordered_set<std::string> funcs;
};

static void collect_sanctions(ast::Node* node, SanctionContext& ctx) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::Program: {
            auto* program = static_cast<ast::ProgramNode*>(node);
            for (auto& stmt : program->statements) collect_sanctions(stmt.get(), ctx);
            break;
        }
        case ast::NodeKind::Block: {
            auto* block = static_cast<ast::BlockNode*>(node);
            for (auto& stmt : block->statements) collect_sanctions(stmt.get(), ctx);
            break;
        }
        case ast::NodeKind::SanctionBlock: {
            auto* sanction = static_cast<ast::SanctionBlockNode*>(node);
            for (const auto& op : sanction->operators) ctx.operators.insert(op);
            for (const auto& fn : sanction->funcs) ctx.funcs.insert(fn);
            for (const auto& cls : sanction->oop) ctx.funcs.insert(cls);
            break;
        }
        case ast::NodeKind::IfStmt: {
            auto* stmt = static_cast<ast::IfStmtNode*>(node);
            collect_sanctions(stmt->then_block.get(), ctx);
            collect_sanctions(stmt->else_block.get(), ctx);
            break;
        }
        case ast::NodeKind::WhileStmt:
            collect_sanctions(static_cast<ast::WhileStmtNode*>(node)->body.get(), ctx);
            break;
        case ast::NodeKind::ForStmt:
            collect_sanctions(static_cast<ast::ForStmtNode*>(node)->body.get(), ctx);
            break;
        case ast::NodeKind::GenesisDecl:
            collect_sanctions(static_cast<ast::GenesisDeclNode*>(node)->body.get(), ctx);
            break;
        default:
            break;
    }
}

static void validate_expression(ast::Node* node, const SanctionContext& ctx) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::BinaryExpr: {
            auto* expr = static_cast<ast::BinaryExprNode*>(node);
            validate_expression(expr->left.get(), ctx);
            validate_expression(expr->right.get(), ctx);
            std::string sanction = operator_sanction_name(expr->op);
            if (!sanction.empty() && ctx.operators.find(sanction) == ctx.operators.end()) {
                throw std::runtime_error("Sanction denied: operator '" + sanction + "' requires '" + sanction + ";' in sanction block");
            }
            break;
        }
        case ast::NodeKind::UnaryExpr:
            validate_expression(static_cast<ast::UnaryExprNode*>(node)->operand.get(), ctx);
            break;
        case ast::NodeKind::FuncCall: {
            auto* call = static_cast<ast::FuncCallNode*>(node);
            if (!is_builtin_function(call->name) && ctx.funcs.find(call->name) == ctx.funcs.end()) {
                throw std::runtime_error("Sanction denied: call '" + call->name + "()' requires '" + call->name + "();' in sanction block");
            }
            for (auto& arg : call->args) validate_expression(arg.get(), ctx);
            break;
        }
        default:
            break;
    }
}

static void validate_statement(ast::Node* node, const SanctionContext& ctx) {
    if (!node) return;
    switch (node->kind) {
        case ast::NodeKind::Program: {
            auto* program = static_cast<ast::ProgramNode*>(node);
            for (auto& stmt : program->statements) validate_statement(stmt.get(), ctx);
            break;
        }
        case ast::NodeKind::Block: {
            auto* block = static_cast<ast::BlockNode*>(node);
            for (auto& stmt : block->statements) validate_statement(stmt.get(), ctx);
            break;
        }
        case ast::NodeKind::VarDecl:
            validate_expression(static_cast<ast::VarDeclNode*>(node)->initializer.get(), ctx);
            break;
        case ast::NodeKind::ReturnStmt:
            validate_expression(static_cast<ast::ReturnStmtNode*>(node)->value.get(), ctx);
            break;
        case ast::NodeKind::IfStmt: {
            auto* stmt = static_cast<ast::IfStmtNode*>(node);
            validate_expression(stmt->condition.get(), ctx);
            validate_statement(stmt->then_block.get(), ctx);
            validate_statement(stmt->else_block.get(), ctx);
            break;
        }
        case ast::NodeKind::WhileStmt: {
            auto* stmt = static_cast<ast::WhileStmtNode*>(node);
            validate_expression(stmt->condition.get(), ctx);
            validate_statement(stmt->body.get(), ctx);
            break;
        }
        case ast::NodeKind::ForStmt: {
            auto* stmt = static_cast<ast::ForStmtNode*>(node);
            validate_expression(stmt->bound.get(), ctx);
            validate_statement(stmt->body.get(), ctx);
            break;
        }
        case ast::NodeKind::FuncCall:
        case ast::NodeKind::Identifier:
            validate_expression(node, ctx);
            break;
        case ast::NodeKind::GenesisDecl:
            validate_statement(static_cast<ast::GenesisDeclNode*>(node)->body.get(), ctx);
            break;
        default:
            break;
    }
}

void validate_sanctions(ast::ProgramNode* program) {
    SanctionContext ctx;
    collect_sanctions(program, ctx);
    validate_statement(program, ctx);
}

std::unique_ptr<ast::ProgramNode> parse_source(const std::string& src) {
    auto tokens = lexer::tokenize(src);
    Parser parser(tokens);
    return parser.parse();
}

}
