#include "module_loader.h"
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

void ModuleLoader::register_native(const std::string& name, vm::VirtualMachine::NativeFunc func) {
    native_functions_[name] = func;
}

vm::VirtualMachine::NativeFunc ModuleLoader::get_function(const std::string& name) const {
    auto it = native_functions_.find(name);
    if (it != native_functions_.end()) {
        return it->second;
    }
    return nullptr;
}

const std::vector<FunctionSignature>& ModuleLoader::get_functions(const std::string& module) const {
    static std::vector<FunctionSignature> empty;
    auto it = module_functions_.find(module);
    if (it != module_functions_.end()) {
        return it->second;
    }
    return empty;
}

std::vector<std::string> ModuleLoader::get_loaded_modules() const {
    std::vector<std::string> modules;
    for (const auto& [name, loaded] : loaded_modules_) {
        if (loaded) modules.push_back(name);
    }
    return modules;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
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
    
    std::vector<FunctionSignature> funcs;
    size_t pos = 0;
    
    while (pos < src.size()) {
        size_t func_pos = src.find("func ", pos);
        if (func_pos == std::string::npos) break;
        
        size_t name_start = func_pos + 5;
        while (name_start < src.size() && std::isspace(src[name_start])) name_start++;
        
        size_t paren = src.find('(', name_start);
        if (paren == std::string::npos) break;
        
        std::string func_name = trim(src.substr(name_start, paren - name_start));
        
        size_t paren_end = src.find(')', paren);
        if (paren_end == std::string::npos) break;
        
        std::string params_str = src.substr(paren + 1, paren_end - paren - 1);
        std::vector<std::string> param_types;
        
        size_t ppos = 0;
        while (ppos < params_str.size()) {
            size_t colon = params_str.find(':', ppos);
            if (colon == std::string::npos) break;
            size_t next = params_str.find(',', colon);
            if (next == std::string::npos) next = params_str.size();
            std::string type = trim(params_str.substr(colon + 1, next - colon - 1));
            if (!type.empty()) param_types.push_back(type);
            ppos = next + 1;
        }
        
        std::string return_type = "i32";
        size_t arrow = src.find("->", paren_end);
        if (arrow != std::string::npos) {
            size_t type_start = arrow + 2;
            while (type_start < src.size() && std::isspace(src[type_start])) type_start++;
            size_t type_end = src.find('\n', type_start);
            if (type_end == std::string::npos) type_end = src.size();
            return_type = trim(src.substr(type_start, type_end - type_start));
        }
        
        funcs.push_back({func_name, param_types, return_type});
        pos = paren_end + 1;
    }
    
    module_functions_[name] = funcs;
    loaded_modules_[name] = true;
    return true;
}

bool ModuleLoader::is_loaded(const std::string& name) const {
    auto it = loaded_modules_.find(name);
    return it != loaded_modules_.end() && it->second;
}

}
