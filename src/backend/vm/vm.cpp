#include "vm.h"
#include <iostream>
#include <unordered_set>

namespace vm {

VirtualMachine::VirtualMachine() : pc_(0), sp_(0), cmp_result_(0), next_function_index_(0) {
    for (int i = 0; i < 16; i++) registers_[i] = 0;
}

void VirtualMachine::set_strings(const std::vector<std::string>& pool) {
    string_pool_ = pool;
}

void VirtualMachine::add_string(const std::string& str) {
    string_pool_.push_back(str);
}

void VirtualMachine::load(const std::vector<Instruction>& bytecode) {
    memory_ = bytecode;
}

void VirtualMachine::register_function(const std::string& name, NativeFunc func, int argc) {
    function_table_[name] = {func, argc};
    function_names_[next_function_index_++] = name;
}

VirtualMachine::NativeFunc VirtualMachine::get_function(const std::string& name, int& argc) const {
    auto it = function_table_.find(name);
    if (it != function_table_.end()) {
        argc = it->second.second;
        return it->second.first;
    }
    argc = 0;
    return nullptr;
}

void VirtualMachine::register_class(const std::string& name, int field_count) {
    class_table_[name] = field_count;
    class_fields_[name] = {};
}

void VirtualMachine::set_class_field(const std::string& class_name, const std::string& field_name, int value) {
    auto class_it = class_fields_.find(class_name);
    if (class_it != class_fields_.end()) {
        class_it->second[field_name] = value;
    }
}

int VirtualMachine::get_class_field(const std::string& class_name, const std::string& field_name) const {
    auto class_it = class_fields_.find(class_name);
    if (class_it != class_fields_.end()) {
        auto field_it = class_it->second.find(field_name);
        if (field_it != class_it->second.end()) {
            return field_it->second;
        }
    }
    return 0;
}

void VirtualMachine::register_sanction(const std::string& type, const std::string& name) {
    if (type == "operator") {
        sanctioned_operators_["default"].insert(name);
    } else if (type == "func") {
        sanctioned_funcs_["default"].insert(name);
    }
}

bool VirtualMachine::is_sanctioned(const std::string& type, const std::string& name) const {
    if (type == "operator") {
        auto it = sanctioned_operators_.find("default");
        if (it != sanctioned_operators_.end()) {
            return it->second.find(name) != it->second.end();
        }
    } else if (type == "func") {
        auto it = sanctioned_funcs_.find("default");
        if (it != sanctioned_funcs_.end()) {
            return it->second.find(name) != it->second.end();
        }
    }
    return false;
}

void VirtualMachine::run(int max_instructions) {
    int instructions_executed = 0;
    while (pc_ < memory_.size() && instructions_executed < max_instructions) {
        Instruction& instr = memory_[pc_++];
        instructions_executed++;
        switch (instr.opcode) {
            case Opcode::NOP: break;
            case Opcode::MOV:
                registers_[instr.arg1] = registers_[instr.arg2];
                break;
            case Opcode::IMM:
                registers_[instr.arg1] = instr.arg2;
                break;
            case Opcode::ADD:
                registers_[instr.arg1] = registers_[instr.arg2] + registers_[instr.arg3];
                break;
            case Opcode::SUB:
                registers_[instr.arg1] = registers_[instr.arg2] - registers_[instr.arg3];
                break;
            case Opcode::MUL:
                registers_[instr.arg1] = registers_[instr.arg2] * registers_[instr.arg3];
                break;
            case Opcode::CMP:
                if (registers_[instr.arg1] < registers_[instr.arg2]) cmp_result_ = -1;
                else if (registers_[instr.arg1] == registers_[instr.arg2]) cmp_result_ = 0;
                else cmp_result_ = 1;
                break;
            case Opcode::JMP:
                pc_ = instr.arg2;
                break;
            case Opcode::JEQ:
                if (cmp_result_ == 0) pc_ = instr.arg2;
                break;
            case Opcode::JNE:
                if (cmp_result_ != 0) pc_ = instr.arg2;
                break;
            case Opcode::JLT:
                if (cmp_result_ < 0) pc_ = instr.arg2;
                break;
            case Opcode::JGT:
                if (cmp_result_ > 0) pc_ = instr.arg2;
                break;
            case Opcode::JLE:
                if (cmp_result_ <= 0) pc_ = instr.arg2;
                break;
            case Opcode::JGE:
                if (cmp_result_ >= 0) pc_ = instr.arg2;
                break;
            case Opcode::DIV:
                if (instr.arg3 != 0)
                    registers_[instr.arg1] = registers_[instr.arg2] / instr.arg3;
                break;
            case Opcode::MOD:
                if (instr.arg3 != 0)
                    registers_[instr.arg1] = registers_[instr.arg2] % instr.arg3;
                break;
            case Opcode::PRINT:
                if (instr.arg2 == 0) {
                    int val = registers_[instr.arg1];
                    if (val >= 1000) {
                        int str_idx = val - 1000;
                        if (str_idx >= 0 && str_idx < (int)string_pool_.size()) {
                            std::cout << string_pool_[str_idx] << "\n";
                        }
                    } else {
                        std::cout << val << "\n";
                    }
                } else {
                    std::cout << "\n";
                }
                break;
            case Opcode::CALL: {
                int func_idx = instr.arg2;
                int argc = instr.arg3;
                int return_reg = instr.arg1;
                if (func_idx >= 0 && func_idx < (int)string_pool_.size()) {
                    std::string func_name = string_pool_[func_idx];
                    int func_argc = 0;
                    auto func = get_function(func_name, func_argc);
                    if (func) {
                        std::vector<int> args(argc);
                        for (int i = 0; i < argc && i < 16; i++) {
                            args[i] = registers_[i + 1];
                        }
                        int result = func(this, argc, args.data());
                        if (return_reg >= 0 && return_reg < 16) {
                            registers_[return_reg] = result;
                        }
                    }
                }
                break;
            }
            case Opcode::PUSH:
                variables_[std::to_string(sp_)] = registers_[instr.arg1];
                sp_++;
                break;
            case Opcode::POP:
                if (sp_ > 0) {
                    sp_--;
                    registers_[instr.arg1] = variables_[std::to_string(sp_)];
                    variables_.erase(std::to_string(sp_));
                }
                break;
            case Opcode::RET:
                pc_ = memory_.size();
                break;
            case Opcode::HLT:
                pc_ = memory_.size();
                break;
            case Opcode::NEW: {
                int class_idx = instr.arg2;
                int dest_reg = instr.arg1;
                if (class_idx >= 0 && class_idx < (int)string_pool_.size()) {
                    std::string class_name = string_pool_[class_idx];
                    int instance_id = class_table_.size() + 1;
                    class_table_[class_name] = instance_id;
                    class_fields_[class_name] = {};
                    if (dest_reg >= 0 && dest_reg < 16) {
                        registers_[dest_reg] = instance_id;
                    }
                }
                break;
            }
            case Opcode::SETFIELD: {
                int instance_id = registers_[instr.arg1];
                int field_idx = instr.arg2;
                int value = registers_[instr.arg3];
                auto it = class_fields_.begin();
                std::advance(it, instance_id - 1);
                if (it != class_fields_.end()) {
                    it->second[std::to_string(field_idx)] = value;
                }
                break;
            }
            case Opcode::GETFIELD: {
                int instance_id = registers_[instr.arg1];
                int field_idx = instr.arg2;
                int dest_reg = instr.arg3;
                auto it = class_fields_.begin();
                std::advance(it, instance_id - 1);
                int value = 0;
                if (it != class_fields_.end()) {
                    auto field_it = it->second.find(std::to_string(field_idx));
                    if (field_it != it->second.end()) {
                        value = field_it->second;
                    }
                }
                if (dest_reg >= 0 && dest_reg < 16) {
                    registers_[dest_reg] = value;
                }
                break;
            }
            case Opcode::SANCTION_CHECK: {
                int check_type = instr.arg1;
                int name_idx = instr.arg2;
                if (name_idx >= 0 && name_idx < (int)string_pool_.size()) {
                    std::string name = string_pool_[name_idx];
                    bool sanctioned = false;
                    if (check_type == 0) {
                        sanctioned = is_sanctioned("operator", name);
                    } else if (check_type == 1) {
                        sanctioned = is_sanctioned("func", name);
                    }
                    if (!sanctioned) {
                        std::string type_str = (check_type == 0) ? "operator" : "call";
                        std::cerr << "VM Sanction denied: " << type_str << " '" << name
                                  << "' not found in Sanction block\n";
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void VirtualMachine::print_registers() const {
    for (int i = 0; i < 16; i++) {
        std::cout << "R" << i << " = " << registers_[i] << "\n";
    }
}

}
