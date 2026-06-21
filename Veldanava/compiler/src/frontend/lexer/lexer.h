#pragma once
#include "token.h"
#include <string>
#include <vector>

namespace lexer {

std::vector<Token> tokenize(const std::string& src);

}