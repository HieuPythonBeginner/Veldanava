#pragma once
#include "../frontend/ast/ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace typechecker {

struct FunctionInfo {
    std::vector<std::pair<std::string, std::string>> params;
    std::string return_type = "i32";
};

struct TypeContext {
    std::unordered_map<std::string, std::string> variables;
    std::unordered_map<std::string, bool> mutable_vars;
    std::unordered_map<std::string, int> borrow_count;
    std::unordered_map<std::string, std::string> class_fields;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> class_field_order;
    std::unordered_map<std::string, std::unordered_map<std::string, FunctionInfo>> class_methods;
    std::unordered_set<std::string> class_names;
    std::unordered_map<std::string, FunctionInfo> functions;
    std::string current_function_return_type;
    std::vector<std::string> errors;
};

std::vector<std::string> type_check(ast::ProgramNode* program);

}
