#pragma once
#include <memory>
#include "ast/ast.h"

namespace parser {
    std::unique_ptr<ast::ProgramNode> parse_source(const std::string& src);
}
