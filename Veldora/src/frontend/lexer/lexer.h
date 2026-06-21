#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <cctype>
#include <stdexcept>

namespace veldora {

enum class TokenType {
    Ident,
    Number,
    String,
    Let,
    Be,
    Add,
    Subtract,
    Multiply,
    Divide,
    Equals,
    NotEquals,
    LessThan,
    GreaterThan,
    LessOrEqual,
    GreaterOrEqual,
    And,
    Or,
    Not,
    If,
    Then,
    Else,
    End,
    While,
    Begin,
    Say,
    Func,
    Return,
    LeftParen,
    RightParen,
    Comma,
    Colon,
    Newline,
    Indent,
    Dedent,
    Eof,
    Error
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int col;
    Token(TokenType t, const std::string& l, int ln, int c)
        : type(t), lexeme(l), line(ln), col(c) {}
};

std::vector<Token> tokenize(const std::string& src);

}
