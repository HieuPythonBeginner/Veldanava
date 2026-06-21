#pragma once
#include <vector>
#include "ast/ast.h"

namespace interpreter {
    int eval(ast::ProgramNode* program);
}
