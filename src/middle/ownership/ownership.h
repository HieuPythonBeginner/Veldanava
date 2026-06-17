#pragma once
#include "../frontend/ast/ast.h"

namespace ownership {

void init();
void enter_scope();
void exit_scope();
void register_mutable(const std::string& name);
void register_borrow(const std::string& name);
bool is_mutable(const std::string& name);
int borrow_count(const std::string& name);
void validate_ast(ast::Node* node);

}
