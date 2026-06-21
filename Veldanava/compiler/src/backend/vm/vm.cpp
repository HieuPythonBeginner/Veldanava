#include "vm.h"
#include <iostream>
#include <unordered_set>
#include <cmath>

namespace vm {

VirtualMachine::VirtualMachine() : pc_(0), sp_(0), cmp_result_(0), next_instance_id_(1), next_array_id_(0) {
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
    function_names_[next_instance_id_++] = name;
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
    class_layout_[name] = std::vector<std::string>(field_count);
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
                int return_reg = instr.arg1;
                int target = instr.arg2;
                int argc = instr.arg3;
                int result = 0;

                if (target >= 1000) {
                    int func_idx = target - 1000;
                    if (func_idx >= 0 && func_idx < (int)string_pool_.size()) {
                        std::string func_name = string_pool_[func_idx];
                        int func_argc = 0;
                        auto func = get_function(func_name, func_argc);
                        if (func) {
                            std::vector<int> args(argc);
                            for (int i = 0; i < argc && i < 16; i++) {
                                args[i] = registers_[i + 1];
                            }
                            result = func(this, argc, args.data());
                        }
                    }
                } else {
                    int saved_pc = pc_;
                    pc_ = target;
                    bool returned = false;
                    while (pc_ < memory_.size() && !returned) {
                        Instruction& ci = memory_[pc_++];
                        switch (ci.opcode) {
                            case Opcode::RET:
                                result = registers_[0];
                                returned = true;
                                break;
                            case Opcode::NOP: break;
                            case Opcode::MOV:
                                registers_[ci.arg1] = registers_[ci.arg2];
                                break;
                            case Opcode::IMM:
                                registers_[ci.arg1] = ci.arg2;
                                break;
                            case Opcode::ADD:
                                registers_[ci.arg1] = registers_[ci.arg2] + registers_[ci.arg3];
                                break;
                            case Opcode::SUB:
                                registers_[ci.arg1] = registers_[ci.arg2] - registers_[ci.arg3];
                                break;
                            case Opcode::MUL:
                                registers_[ci.arg1] = registers_[ci.arg2] * registers_[ci.arg3];
                                break;
                            case Opcode::DIV:
                                if (ci.arg3 != 0)
                                    registers_[ci.arg1] = registers_[ci.arg2] / ci.arg3;
                                break;
                            case Opcode::MOD:
                                if (ci.arg3 != 0)
                                    registers_[ci.arg1] = registers_[ci.arg2] % ci.arg3;
                                break;
                            case Opcode::CMP:
                                if (registers_[ci.arg1] < registers_[ci.arg2]) cmp_result_ = -1;
                                else if (registers_[ci.arg1] == registers_[ci.arg2]) cmp_result_ = 0;
                                else cmp_result_ = 1;
                                break;
                            case Opcode::JMP:
                                pc_ = ci.arg2;
                                break;
                            case Opcode::JEQ:
                                if (cmp_result_ == 0) pc_ = ci.arg2;
                                break;
                            case Opcode::JNE:
                                if (cmp_result_ != 0) pc_ = ci.arg2;
                                break;
                            case Opcode::JLT:
                                if (cmp_result_ < 0) pc_ = ci.arg2;
                                break;
                            case Opcode::JGT:
                                if (cmp_result_ > 0) pc_ = ci.arg2;
                                break;
                            case Opcode::JLE:
                                if (cmp_result_ <= 0) pc_ = ci.arg2;
                                break;
                            case Opcode::JGE:
                                if (cmp_result_ >= 0) pc_ = ci.arg2;
                                break;
                            case Opcode::LOAD:
                                registers_[ci.arg2] = registers_[registers_[ci.arg1]];
                                break;
                            case Opcode::STORE:
                                registers_[ci.arg1] = registers_[ci.arg2];
                                break;
                            case Opcode::ARR_NEW: {
                                int array_id = next_array_id_++;
                                int size = ci.arg2;
                                arrays_[array_id] = std::vector<int>(size, 0);
                                registers_[ci.arg1] = array_id;
                                break;
                            }
                            case Opcode::ARR_GET: {
                                int array_id = registers_[ci.arg1];
                                int index = registers_[ci.arg2];
                                registers_[ci.arg3] = 0;
                                auto it = arrays_.find(array_id);
                                if (it != arrays_.end() && index >= 0 && index < (int)it->second.size()) {
                                    registers_[ci.arg3] = it->second[index];
                                }
                                break;
                            }
                            case Opcode::ARR_SET: {
                                int array_id = registers_[ci.arg1];
                                int index = ci.arg2;
                                int value = registers_[ci.arg3];
                                auto it = arrays_.find(array_id);
                                if (it != arrays_.end() && index >= 0 && index < (int)it->second.size()) {
                                    it->second[index] = value;
                                }
                                break;
                            }
                            case Opcode::ARR_LEN: {
                                int array_id = registers_[ci.arg1];
                                int len = 0;
                                auto it = arrays_.find(array_id);
                                if (it != arrays_.end()) {
                                    len = (int)it->second.size();
                                }
                                registers_[ci.arg2] = len;
                                break;
                            }
                            case Opcode::NEW: {
                                int class_idx = ci.arg2;
                                int dest_reg = ci.arg1;
                                if (class_idx >= 0 && class_idx < (int)string_pool_.size()) {
                                    std::string class_name = string_pool_[class_idx];
                                    int instance_id = next_instance_id_++;
                                    object_classes_[instance_id] = class_name;
                                    object_fields_[instance_id] = {};
                                    int field_count = ci.arg3;
                                    for (int i = 0; i < field_count; i++) {
                                        object_fields_[instance_id][i] = 0;
                                    }
                                    if (dest_reg >= 0 && dest_reg < 16) {
                                        registers_[dest_reg] = instance_id;
                                    }
                                }
                                break;
                            }
                            case Opcode::SETFIELD: {
                                int instance_id = registers_[ci.arg1];
                                int field_idx = ci.arg2;
                                int value = registers_[ci.arg3];
                                object_fields_[instance_id][field_idx] = value;
                                break;
                            }
                            case Opcode::GETFIELD: {
                                int instance_id = registers_[ci.arg1];
                                int field_idx = ci.arg2;
                                int dest_reg = ci.arg3;
                                int value = 0;
                                auto object_it = object_fields_.find(instance_id);
                                if (object_it != object_fields_.end()) {
                                    auto field_it = object_it->second.find(field_idx);
                                    if (field_it != object_it->second.end()) {
                                        value = field_it->second;
                                    }
                                }
                                if (dest_reg >= 0 && dest_reg < 16) {
                                    registers_[dest_reg] = value;
                                }
                                break;
                            }
                            case Opcode::SQRT: {
                                int src = ci.arg2;
                                int dst = ci.arg1;
                                registers_[dst] = (int)std::sqrt((double)registers_[src]);
                                break;
                            }
                            case Opcode::ABS: {
                                int src = ci.arg2;
                                int dst = ci.arg1;
                                registers_[dst] = std::abs(registers_[src]);
                                break;
                            }
                            case Opcode::FLOOR: {
                                int src = ci.arg2;
                                int dst = ci.arg1;
                                registers_[dst] = (int)std::floor((double)registers_[src]);
                                break;
                            }
                            case Opcode::CEIL: {
                                int src = ci.arg2;
                                int dst = ci.arg1;
                                registers_[dst] = (int)std::ceil((double)registers_[src]);
                                break;
                            }
                            case Opcode::POW: {
                                int base = ci.arg2;
                                int exp = ci.arg3;
                                int dst = ci.arg1;
                                registers_[dst] = (int)std::pow((double)registers_[base], (double)registers_[exp]);
                                break;
                            }
                            case Opcode::SIN: {
                                int src = ci.arg2;
                                int dst = ci.arg1;
                                registers_[dst] = (int)std::sin((double)registers_[src]);
                                break;
                            }
                            case Opcode::COS: {
                                int src = ci.arg2;
                                int dst = ci.arg1;
                                registers_[dst] = (int)std::cos((double)registers_[src]);
                                break;
                            }
                            case Opcode::TAN: {
                                int src = ci.arg2;
                                int dst = ci.arg1;
                                registers_[dst] = (int)std::tan((double)registers_[src]);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    pc_ = saved_pc;
                }

                if (return_reg >= 0 && return_reg < 16) {
                    registers_[return_reg] = result;
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
            case Opcode::LOAD:
                registers_[instr.arg2] = registers_[registers_[instr.arg1]];
                break;
            case Opcode::STORE:
                registers_[registers_[instr.arg1]] = registers_[instr.arg2];
                break;
            case Opcode::ARR_NEW: {
                int array_id = next_array_id_++;
                int size = instr.arg2;
                std::vector<int> elems(size, 0);
                arrays_[array_id] = elems;
                if (instr.arg1 >= 0 && instr.arg1 < 16) {
                    registers_[instr.arg1] = array_id;
                }
                break;
            }
            case Opcode::ARR_GET: {
                int array_id = registers_[instr.arg1];
                int index = registers_[instr.arg2];
                int dest_reg = instr.arg3;
                int value = 0;
                auto it = arrays_.find(array_id);
                if (it != arrays_.end() && index >= 0 && index < (int)it->second.size()) {
                    value = it->second[index];
                }
                if (dest_reg >= 0 && dest_reg < 16) {
                    registers_[dest_reg] = value;
                }
                break;
            }
            case Opcode::ARR_SET: {
                int array_id = registers_[instr.arg1];
                int index = instr.arg2;
                int value = registers_[instr.arg3];
                auto it = arrays_.find(array_id);
                if (it != arrays_.end() && index >= 0 && index < (int)it->second.size()) {
                    it->second[index] = value;
                }
                break;
            }
            case Opcode::ARR_LEN: {
                int array_id = registers_[instr.arg1];
                int dest_reg = instr.arg3;
                int len = 0;
                auto it = arrays_.find(array_id);
                if (it != arrays_.end()) {
                    len = (int)it->second.size();
                }
                if (dest_reg >= 0 && dest_reg < 16) {
                    registers_[dest_reg] = len;
                }
                break;
            }
            case Opcode::NEW: {
                int class_idx = instr.arg2;
                int dest_reg = instr.arg1;
                if (class_idx >= 0 && class_idx < (int)string_pool_.size()) {
                    std::string class_name = string_pool_[class_idx];
                    int instance_id = next_instance_id_++;
                    object_classes_[instance_id] = class_name;
                    object_fields_[instance_id] = {};
                    int field_count = instr.arg3;
                    for (int i = 0; i < field_count; i++) {
                        object_fields_[instance_id][i] = 0;
                    }
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
                object_fields_[instance_id][field_idx] = value;
                break;
            }
            case Opcode::GETFIELD: {
                int instance_id = registers_[instr.arg1];
                int field_idx = instr.arg2;
                int dest_reg = instr.arg3;
                int value = 0;
                auto object_it = object_fields_.find(instance_id);
                if (object_it != object_fields_.end()) {
                    auto field_it = object_it->second.find(field_idx);
                    if (field_it != object_it->second.end()) {
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
            case Opcode::SQRT: {
                int src = instr.arg2;
                int dst = instr.arg1;
                registers_[dst] = (int)std::sqrt((double)registers_[src]);
                break;
            }
            case Opcode::ABS: {
                int src = instr.arg2;
                int dst = instr.arg1;
                registers_[dst] = std::abs(registers_[src]);
                break;
            }
            case Opcode::FLOOR: {
                int src = instr.arg2;
                int dst = instr.arg1;
                registers_[dst] = (int)std::floor((double)registers_[src]);
                break;
            }
            case Opcode::CEIL: {
                int src = instr.arg2;
                int dst = instr.arg1;
                registers_[dst] = (int)std::ceil((double)registers_[src]);
                break;
            }
            case Opcode::POW: {
                int base = instr.arg2;
                int exp = instr.arg3;
                int dst = instr.arg1;
                registers_[dst] = (int)std::pow((double)registers_[base], (double)registers_[exp]);
                break;
            }
            case Opcode::SIN: {
                int src = instr.arg2;
                int dst = instr.arg1;
                registers_[dst] = (int)std::sin((double)registers_[src]);
                break;
            }
            case Opcode::COS: {
                int src = instr.arg2;
                int dst = instr.arg1;
                registers_[dst] = (int)std::cos((double)registers_[src]);
                break;
            }
            case Opcode::TAN: {
                int src = instr.arg2;
                int dst = instr.arg1;
                registers_[dst] = (int)std::tan((double)registers_[src]);
                break;
            }
            default:
                break;
        }
    }
    last_instruction_count_ = instructions_executed;
}

void VirtualMachine::print_registers() const {
    for (int i = 0; i < 16; i++) {
        std::cout << "R" << i << " = " << registers_[i] << "\n";
    }
}

}
