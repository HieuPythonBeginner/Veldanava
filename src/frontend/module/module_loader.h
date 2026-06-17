#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ast {
    struct ProgramNode;
}

namespace module {

struct FunctionSignature {
    std::string name;
    std::vector<std::string> param_types;
    std::string return_type;
};

class ModuleLoader {
public:
    static ModuleLoader& instance();
    
    bool load_module(const std::string& name);
    bool is_loaded(const std::string& name) const;
    
    const std::vector<FunctionSignature>& get_functions(const std::string& module) const;
    
    std::vector<std::string> get_loaded_modules() const;
    const ast::ProgramNode* get_module_ast(const std::string& name) const;

private:
    ModuleLoader() = default;
    std::unordered_map<std::string, bool> loaded_modules_;
    std::unordered_map<std::string, std::vector<FunctionSignature>> module_functions_;
    std::unordered_map<std::string, std::unique_ptr<ast::ProgramNode>> module_asts_;
};

}
