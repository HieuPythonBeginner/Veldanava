#pragma once
#include <string>

namespace lexer {

enum class TokenType {
    Eof, Error,
    IntLit, FloatLit, StringLit, CharLit, Ident,
    Add, Sub, Mul, Div, Mod,
    Assign, Eq, Ne, Lt, Gt, Le, Ge,
    And, Or, Not,
    RArrow,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semi, Colon, Dot,
    If, Else, While, For, Return, Break, Continue,
    In,
    Let, Const, Func, Class, Struct,
    I32, I64, U32, U64, F32, F64, Bool, Void, String, Char,
    AndKw, OrKw, NotKw,
    Indent, Dedent,
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int col;

    Token(TokenType t, const std::string& l, int ln = 0, int c = 0)
        : type(t), lexeme(l), line(ln), col(c) {}
};

}