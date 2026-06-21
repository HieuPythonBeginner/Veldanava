#include "parser.h"
#include "../ast/ast.h"
namespace parser {
    std::unique_ptr<ast::ProgramNode> parse_source(const std::string& src) {
        auto prog = std::make_unique<ast::ProgramNode>();
        return prog;
    }
}
