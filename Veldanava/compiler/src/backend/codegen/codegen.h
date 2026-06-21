#pragma once
#include "vm/instruction.h"
#include "ast/ast.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace codegen {

class CodeGenerator {
public:
    std::vector<vm::Instruction> generate(ast::ProgramNode* program);
    const std::vector<std::string>& get_string_pool() const { return string_pool_; }

    int emit(vm::Opcode op, int a1 = 0, int a2 = 0, int a3 = 0);
    int emit_jmp(vm::Opcode op, int target);
    void patch(int pos, int target);
    int current_pos() const;

private:
    std::vector<vm::Instruction> instructions_;
    int next_reg_;
    int next_array_id_;
    std::unordered_map<std::string, int> function_table_;

    std::unordered_map<std::string, int> var_regs_;
    std::vector<std::unordered_map<std::string, int>> var_scope_stack_;

    std::vector<int> break_patches_;
    std::vector<int> continue_patches_;
    std::vector<std::string> string_pool_;
    std::unordered_map<std::string, int> string_indices_;
    std::unordered_set<std::string> incorporated_modules_;
    std::unordered_set<std::string> class_names_;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> class_fields_;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> field_indices_;

    int add_string(const std::string& s);
    void incorporate_module(const std::string& module);
    bool is_module_function_available(const std::string& name) const;

    void gen_stmt(ast::Node* node);
    int gen_expr(ast::Node* node);
    int gen_lvalue_address(ast::Node* node);
    int emit_condition_jump(ast::Node* node);

    void push_scope();
    void pop_scope();
    int declare_var(const std::string& name);
    int get_var_reg(const std::string& name);

    void collect_class_decl(ast::GenesisDeclNode* decl);
    void generate_function(ast::Node* node);
    std::string method_key(const std::string& class_name, const std::string& method_name) const;
};

std::vector<vm::Instruction> generate_code(ast::ProgramNode* program);

}
