#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include "../../frontend/ast/ast.h"

namespace veldora {

class Interpreter {
public:
    Interpreter();
    int eval(ProgramNode* program);

private:
    std::unordered_map<std::string, int> variables_;
    std::unordered_map<std::string, int> functions_;
    std::vector<std::unordered_map<std::string, int>> scopes_;

    void enter_scope();
    void exit_scope();
    int eval_expr(ExprNode* node);
    int eval_stmt(StmtNode* node);
};

}
