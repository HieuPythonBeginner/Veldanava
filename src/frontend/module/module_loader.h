#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../backend/vm/vm.h"

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
    
    void register_native(const std::string& name, vm::VirtualMachine::NativeFunc func);
    vm::VirtualMachine::NativeFunc get_function(const std::string& name) const;
    const std::vector<FunctionSignature>& get_functions(const std::string& module) const;
    
    std::vector<std::string> get_loaded_modules() const;

private:
    ModuleLoader() = default;
    std::unordered_map<std::string, bool> loaded_modules_;
    std::unordered_map<std::string, std::vector<FunctionSignature>> module_functions_;
    std::unordered_map<std::string, vm::VirtualMachine::NativeFunc> native_functions_;
};

}
