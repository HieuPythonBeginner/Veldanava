#include "lexer.h"
#include "keyword_table.h"
#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace lexer {

class LexerImpl {
public:
    LexerImpl(const std::string& src) : src_(src), pos_(0), line_(1), col_(0), at_line_start_(true) {
        indent_stack_.push_back(0);
    }

    Token next_token() {
        while (true) {
            if (dedent_pending_) {
                dedent_pending_ = false;
                return Token(TokenType::Dedent, "", line_, col_);
            }

            if (pos_ >= src_.size()) return Token(TokenType::Eof, "", line_, col_);

            char c = src_[pos_];

            if (c == '\n') {
                line_++;
                col_ = 0;
                pos_++;
                at_line_start_ = true;
                continue;
            }

            if (at_line_start_) {
                // Count indentation at start of line
                int current_indent = 0;
                while (pos_ < src_.size()) {
                    char c = src_[pos_];
                    if (c == ' ') { current_indent++; pos_++; }
                    else if (c == '\t') { current_indent += 4; pos_++; }
                    else if (c == '\n') {
                        line_++; col_ = 0; pos_++;
                        // Blank line - continue to next line
                        current_indent = 0;
                        continue;
                    }
                    else break;
                }

                int prev_indent = indent_stack_.back();

                if (current_indent > prev_indent) {
                    indent_stack_.push_back(current_indent);
                    col_ = current_indent;
                    at_line_start_ = false;
                    return Token(TokenType::Indent, "", line_, col_);
                } else if (current_indent < prev_indent) {
                    // Dedent needed - pop to find matching level
                    indent_stack_.pop_back();
                    col_ = 0;
                    at_line_start_ = false;
                    return Token(TokenType::Dedent, "", line_, col_);
                }
                // Equal indent: position col and continue
                col_ = current_indent;
                at_line_start_ = false;
            }

            if (pos_ >= src_.size()) return Token(TokenType::Eof, "", line_, col_);

            c = src_[pos_];

            if (c == ' ' || c == '\t' || c == '\r') {
                col_++; pos_++;
                continue;
            }

            break;
        }

        char c = src_[pos_];

        if (std::isdigit(c)) return scan_number();
        if (std::isalpha(c) || c == '_') return scan_identifier();

        switch (c) {
            case '+': col_++; pos_++; return Token(TokenType::Add, "+");
            case '*': col_++; pos_++; return Token(TokenType::Mul, "*");
            case '/': col_++; pos_++; return Token(TokenType::Div, "/");
            case '%': col_++; pos_++; return Token(TokenType::Mod, "%");
            case '=': return scan_eq();
            case '!': return scan_ne();
            case '<': return scan_lt();
            case '>': return scan_gt();
            case '&': col_++; pos_++; return Token(TokenType::And, "&");
            case '|': col_++; pos_++; return Token(TokenType::Or, "|");
            case '~': col_++; pos_++; return Token(TokenType::Not, "~");
            case '(': col_++; pos_++; return Token(TokenType::LParen, "(");
            case ')': col_++; pos_++; return Token(TokenType::RParen, ")");
            case '{': col_++; pos_++; return Token(TokenType::LBrace, "{");
            case '}': col_++; pos_++; return Token(TokenType::RBrace, "}");
            case '[': col_++; pos_++; return Token(TokenType::LBracket, "[");
            case ']': col_++; pos_++; return Token(TokenType::RBracket, "]");
            case ',': col_++; pos_++; return Token(TokenType::Comma, ",");
            case ';': col_++; pos_++; return Token(TokenType::Semi, ";");
            case ':': col_++; pos_++; return Token(TokenType::Colon, ":");
            case '.': col_++; pos_++; return Token(TokenType::Dot, ".");
            case '-': return scan_arrow();
            case '"': return scan_string();
            case '\'': return scan_char();
            default:
                col_++; pos_++;
                return Token(TokenType::Error, std::string(1, c));
        }
    }

private:
    std::string src_;
    size_t pos_;
    int line_, col_;
    bool at_line_start_;
    bool dedent_pending_ = false;
    std::vector<int> indent_stack_;

    Token scan_number() {
        size_t start = pos_;
        bool is_float = false;
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (c == '.') is_float = true;
            else if (!std::isdigit(c)) break;
            pos_++; col_++;
        }
        TokenType type = is_float ? TokenType::FloatLit : TokenType::IntLit;
        return Token(type, src_.substr(start, pos_ - start), line_, col_);
    }

    Token scan_identifier() {
        size_t start = pos_;
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (!std::isalnum(c) && c != '_') break;
            pos_++; col_++;
        }
        std::string text = src_.substr(start, pos_ - start);
        auto keywords = get_keywords();
        auto it = keywords.find(text);
        TokenType type = (it != keywords.end()) ? it->second : TokenType::Ident;
        return Token(type, text, line_, col_);
    }

    Token scan_eq() {
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
            col_ += 2; pos_ += 2;
            return Token(TokenType::Eq, "==");
        }
        col_++; pos_++;
        return Token(TokenType::Assign, "=");
    }

    Token scan_ne() {
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
            col_ += 2; pos_ += 2;
            return Token(TokenType::Ne, "!=");
        }
        col_++; pos_++;
        return Token(TokenType::Error, "!");
    }

    Token scan_lt() {
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
            col_ += 2; pos_ += 2;
            return Token(TokenType::Le, "<=");
        }
        col_++; pos_++;
        return Token(TokenType::Lt, "<");
    }

    Token scan_gt() {
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
            col_ += 2; pos_ += 2;
            return Token(TokenType::Ge, ">=");
        }
        col_++; pos_++;
        return Token(TokenType::Gt, ">");
    }

    Token scan_arrow() {
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '>') {
            col_ += 2; pos_ += 2;
            return Token(TokenType::RArrow, "->");
        }
        col_++; pos_++;
        return Token(TokenType::Sub, "-");
    }

    Token scan_string() {
        pos_++; col_++;
        size_t start = pos_;
        while (pos_ < src_.size() && src_[pos_] != '"') {
            if (src_[pos_] == '\\') pos_++;
            pos_++; col_++;
        }
        std::string val = src_.substr(start, pos_ - start);
        if (pos_ < src_.size()) { pos_++; col_++; }
        return Token(TokenType::StringLit, val, line_, col_);
    }

    Token scan_char() {
        pos_++; col_++;
        size_t start = pos_;
        if (src_[pos_] == '\\') pos_++;
        if (pos_ < src_.size()) pos_++;
        std::string val = src_.substr(start, pos_ - start);
        if (pos_ < src_.size() && src_[pos_] == '\'') { pos_++; col_++; }
        return Token(TokenType::CharLit, val, line_, col_);
    }
};

std::vector<Token> tokenize(const std::string& src) {
    std::vector<Token> tokens;
    LexerImpl lexer(src);
    while (true) {
        Token tok = lexer.next_token();
        tokens.push_back(tok);
        if (tok.type == TokenType::Eof || tok.type == TokenType::Error) break;
    }
    return tokens;
}

}