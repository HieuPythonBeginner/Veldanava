#pragma once
#include <vector>
#include "ast/ast.h"

namespace jit {
    std::vector<int> compile(ast::ProgramNode* program);
}
