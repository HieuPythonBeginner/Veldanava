#include "lexer.h"
#include <cctype>
#include <unordered_map>

namespace veldora {

static std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::Let},
    {"be", TokenType::Be},
    {"add", TokenType::Add},
    {"subtract", TokenType::Subtract},
    {"multiply", TokenType::Multiply},
    {"divide", TokenType::Divide},
    {"equals", TokenType::Equals},
    {"not_equals", TokenType::NotEquals},
    {"less_than", TokenType::LessThan},
    {"greater_than", TokenType::GreaterThan},
    {"less_or_equal", TokenType::LessOrEqual},
    {"greater_or_equal", TokenType::GreaterOrEqual},
    {"and", TokenType::And},
    {"or", TokenType::Or},
    {"not", TokenType::Not},
    {"if", TokenType::If},
    {"then", TokenType::Then},
    {"else", TokenType::Else},
    {"end", TokenType::End},
    {"while", TokenType::While},
    {"begin", TokenType::Begin},
    {"say", TokenType::Say},
    {"func", TokenType::Func},
    {"return", TokenType::Return},
};

static bool is_ident_start(char c) {
    return std::isalpha(c) || c == '_';
}

static bool is_ident_char(char c) {
    return std::isalnum(c) || c == '_';
}

static std::string read_string(const std::string& src, size_t& i, int line, int col) {
    if (src[i] != '"') {
        throw std::runtime_error("Expected '\"' at " + std::to_string(line) + ":" + std::to_string(col));
    }
    i++;
    std::string result;
    while (i < src.size() && src[i] != '"') {
        if (src[i] == '\\' && i + 1 < src.size()) {
            i++;
            char next = src[i++];
            if (next == 'n') result += '\n';
            else if (next == 't') result += '\t';
            else if (next == '"') result += '"';
            else if (next == '\\') result += '\\';
            else { result += '\\'; result += next; }
        } else {
            result += src[i++];
        }
    }
    if (i < src.size()) i++;
    return result;
}

std::vector<Token> tokenize(const std::string& src) {
    std::vector<Token> tokens;
    size_t i = 0;
    int line = 1;
    int col = 1;

    auto make_token = [&](TokenType type, const std::string& lexeme) {
        tokens.emplace_back(type, lexeme, line, col);
        col += static_cast<int>(lexeme.size());
    };

    auto skip_comment = [&]() {
        while (i < src.size() && src[i] != '\n') {
            i++;
            col++;
        }
    };

    while (i < src.size()) {
        char c = src[i];

        if (std::isspace(c)) {
            if (c == '\n') {
                tokens.emplace_back(TokenType::Newline, "\n", line, col);
                line++;
                col = 1;
            } else {
                col++;
            }
            i++;
            continue;
        }

        if (c == '#') {
            skip_comment();
            continue;
        }

        if (c == '"') {
            std::string str = read_string(src, i, line, col);
            tokens.emplace_back(TokenType::String, str, line, col);
            col += static_cast<int>(str.size()) + 2;
            continue;
        }

        if (std::isdigit(c)) {
            size_t start = i;
            while (i < src.size() && std::isdigit(src[i])) {
                i++;
                col++;
            }
            tokens.emplace_back(TokenType::Number, src.substr(start, i - start), line, col - static_cast<int>(i - start));
            continue;
        }

        if (c == '(') { make_token(TokenType::LeftParen, "("); i++; continue; }
        if (c == ')') { make_token(TokenType::RightParen, ")"); i++; continue; }
        if (c == ',') { make_token(TokenType::Comma, ","); i++; continue; }
        if (c == ':') { make_token(TokenType::Colon, ":"); i++; continue; }

        if (is_ident_start(c)) {
            size_t start = i;
            while (i < src.size() && is_ident_char(src[i])) {
                i++;
                col++;
            }
            std::string word = src.substr(start, i - start);
            auto it = keywords.find(word);
            if (it != keywords.end()) {
                tokens.emplace_back(it->second, word, line, col - static_cast<int>(word.size()));
            } else {
                tokens.emplace_back(TokenType::Ident, word, line, col - static_cast<int>(word.size()));
            }
            continue;
        }

        tokens.emplace_back(TokenType::Error, std::string(1, c), line, col);
        i++;
        col++;
    }

    tokens.emplace_back(TokenType::Eof, "", line, col);
    return tokens;
}

}
