#pragma once
#include "../frontend/ast/ast.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace typechecker {

struct TypeContext {
    std::unordered_map<std::string, std::string> variables;
    std::string current_function_return_type;
    std::vector<std::string> errors;
};

std::vector<std::string> type_check(ast::ProgramNode* program);

}
