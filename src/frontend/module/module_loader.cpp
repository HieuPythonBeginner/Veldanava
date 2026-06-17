#include "module_loader.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>

namespace module {

ModuleLoader& ModuleLoader::instance() {
    static ModuleLoader loader;
    return loader;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool ModuleLoader::load_module(const std::string& name) {
    if (loaded_modules_.count(name)) return true;
    
    std::string path = "modules/" + name + ".veldanava";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string src = buffer.str();
    
    std::string transformed = "Primordial_Regalia;\n\n";
    size_t pos = 0;
    while (pos < src.size()) {
        size_t line_end = src.find('\n', pos);
        std::string line = src.substr(pos, line_end - pos);
        
        size_t first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line.substr(first, 5) == "func ") {
            line = line.substr(0, first) + "genesis " + line.substr(first);
        }
        
        transformed += line + "\n";
        pos = (line_end == std::string::npos) ? src.size() : line_end + 1;
    }
    
    try {
        auto ast = parser::parse_source(transformed);
        if (!ast) return false;
        
        std::vector<FunctionSignature> funcs;
        for (auto& stmt : ast->statements) {
            if (stmt->kind == ast::NodeKind::GenesisDecl) {
                auto* decl = static_cast<ast::GenesisDeclNode*>(stmt.get());
                if (decl->kind == "func") {
                    FunctionSignature sig;
                    sig.name = decl->name;
                    for (auto& param : decl->params) {
                        sig.param_types.push_back(param.second);
                    }
                    sig.return_type = decl->return_type.empty() ? "i32" : decl->return_type;
                    funcs.push_back(sig);
                }
            }
        }
        
        module_functions_[name] = funcs;
        module_asts_[name] = std::move(ast);
        loaded_modules_[name] = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Module load error: " << e.what() << "\n";
        return false;
    }
}

const std::vector<FunctionSignature>& ModuleLoader::get_functions(const std::string& module) const {
    static std::vector<FunctionSignature> empty;
    auto it = module_functions_.find(module);
    if (it != module_functions_.end()) {
        return it->second;
    }
    return empty;
}

const ast::ProgramNode* ModuleLoader::get_module_ast(const std::string& name) const {
    auto it = module_asts_.find(name);
    if (it != module_asts_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> ModuleLoader::get_loaded_modules() const {
    std::vector<std::string> modules;
    for (const auto& [name, loaded] : loaded_modules_) {
        if (loaded) modules.push_back(name);
    }
    return modules;
}

bool ModuleLoader::is_loaded(const std::string& name) const {
    auto it = loaded_modules_.find(name);
    return it != loaded_modules_.end() && it->second;
}

}