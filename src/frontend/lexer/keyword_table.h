#pragma once
#include "token.h"
#include <unordered_map>

namespace lexer {

inline std::unordered_map<std::string, TokenType> get_keywords() {
    return {
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"elif", TokenType::Elif},
        {"while", TokenType::While},
        {"for", TokenType::For},
        {"in", TokenType::In},
        {"break", TokenType::Break},
        {"continue", TokenType::Continue},
        {"return", TokenType::Return},
        {"let", TokenType::Let},
        {"const", TokenType::Const},
        {"var", TokenType::Var},
        {"func", TokenType::Func},
        {"class", TokenType::Class},
        {"struct", TokenType::Struct},
        {"genesis", TokenType::Genesis},
        {"Incorporate", TokenType::Incorporate},
        {"Sanction", TokenType::Sanction},
        {"Primordial_Regalia", TokenType::Primordial_Regalia},
        {"i32", TokenType::I32},
        {"i64", TokenType::I64},
        {"u32", TokenType::U32},
        {"u64", TokenType::U64},
        {"f32", TokenType::F32},
        {"f64", TokenType::F64},
        {"bool", TokenType::Bool},
        {"true", TokenType::BoolLit},
        {"false", TokenType::BoolLit},
        {"string", TokenType::String},
        {"char", TokenType::Char},
        {"void", TokenType::Void},
        {"and", TokenType::And},
        {"or", TokenType::Or},
        {"not", TokenType::Not},
    };
}

}