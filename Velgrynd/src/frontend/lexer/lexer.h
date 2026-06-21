#pragma once
#include <string>
#include <vector>

namespace lexer {
    struct Token {
        int type;
        std::string lexeme;
        int line;
        int col;
        Token(int t, std::string l, int ln=0, int c=0) : type(t), lexeme(l), line(ln), col(c) {}
    };
    std::vector<Token> tokenize(const std::string& src);
}
