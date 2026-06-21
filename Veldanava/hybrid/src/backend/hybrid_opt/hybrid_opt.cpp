#include "hybrid_opt.h"
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cmath>
#include <algorithm>

namespace hybrid_opt {

HybridRuntime::HybridRuntime() = default;
HybridRuntime::~HybridRuntime() = default;

int HybridRuntime::execute(ast::ProgramNode* program) {
    program_ = program;
    std::cout << "[HybridOpt] Tier " << static_cast<int>(global_tier_)
              << ": Interpreter baseline\n";
    return tier0_execute(program);
}

int HybridRuntime::tier0_execute(ast::Node* node) {
    if (!node) return 0;
    auto start = std::chrono::high_resolution_clock::now();
    int result = 0;

    switch (node->kind) {
        case ast::NodeKind::Program: {
            auto* prog = static_cast<ast::ProgramNode*>(node);
            for (auto& stmt : prog->statements) {
                result = tier0_execute(stmt.get());
            }
            break;
        }
        case ast::NodeKind::VarDecl: {
            auto* vd = static_cast<ast::VarDeclNode*>(node);
            if (vd->initializer) {
                result = tier0_execute(vd->initializer.get());
                variables_[vd->name] = result;
            }
            break;
        }
        case ast::NodeKind::Literal: {
            auto* lit = static_cast<ast::LiteralNode*>(node);
            if (lit->kind == ast::LiteralKind::Int) {
                result = std::stoi(lit->value);
            } else if (lit->kind == ast::LiteralKind::Bool) {
                result = (lit->value == "true") ? 1 : 0;
            } else if (lit->kind == ast::LiteralKind::Char) {
                result = lit->value.empty() ? 0 : static_cast<unsigned char>(lit->value[0]);
            } else {
                result = 0;
            }
            break;
        }
        case ast::NodeKind::BinaryExpr: {
            auto* bin = static_cast<ast::BinaryExprNode*>(node);
            int left = tier0_execute(bin->left.get());
            int right = tier0_execute(bin->right.get());
            switch (static_cast<int>(bin->op)) {
                case 0: result = left + right; break;
                case 1: result = left - right; break;
                case 2: result = left * right; break;
                case 3: result = (right != 0) ? (left / right) : 0; break;
                case 4: result = (right != 0) ? (left % right) : 0; break;
                case 5: result = (left < right) ? 1 : 0; break;
                case 6: result = (left > right) ? 1 : 0; break;
                case 7: result = (left <= right) ? 1 : 0; break;
                case 8: result = (left >= right) ? 1 : 0; break;
                case 9: result = (left == right) ? 1 : 0; break;
                case 10: result = (left != right) ? 1 : 0; break;
                default: result = left; break;
            }
            break;
        }
        case ast::NodeKind::FuncDef: {
            auto* fd = static_cast<ast::FuncDefNode*>(node);
            variables_[fd->name] = 0;
            break;
        }
        case ast::NodeKind::FuncCall: {
            auto* fc = static_cast<ast::FuncCallNode*>(node);
            std::string cache_key = fc->name + "#" + std::to_string(fc->args.size());

            auto it = inline_caches_.find(cache_key);
            if (it != inline_caches_.end() && it->second.cached_target >= 0) {
                result = it->second.cached_target;
                it->second.hit_count++;
            } else {
                if (fc->name == "print") {
                    for (size_t i = 0; i < fc->args.size(); ++i) {
                        int val = tier0_execute(fc->args[i].get());
                        if (i > 0) std::cout << " ";
                        std::cout << val;
                    }
                    std::cout << "\n";
                    result = 0;
                } else if (fc->name == "sqrt") {
                    result = fc->args.size() > 0 ? static_cast<int>(std::sqrt(tier0_execute(fc->args[0].get()))) : 0;
                } else {
                    result = 0;
                }

                InlineCache ic;
                ic.key = cache_key;
                ic.cached_target = result;
                ic.cached_type = 0;
                ic.hit_count = 1;
                inline_caches_[cache_key] = ic;
            }
            break;
        }
        default:
            break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    record_execution(node,
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    return result;
}

int HybridRuntime::tier1_execute(ast::Node* node) {
    if (!node) return 0;

    if (node->kind == ast::NodeKind::FuncDef) {
        auto* fd = static_cast<ast::FuncDefNode*>(node);
        int func_id = compile_function_ir(fd);
        return execute_ir(ir_, func_id);
    }

    if (node->kind == ast::NodeKind::FuncCall) {
        auto* fc = static_cast<ast::FuncCallNode*>(node);
        std::string cache_key = fc->name + "#" + std::to_string(fc->args.size());
        auto it = inline_caches_.find(cache_key);
        if (it != inline_caches_.end() && it->second.cached_target >= 0) {
            return it->second.cached_target;
        }
    }

    return tier0_execute(node);
}

int HybridRuntime::tier2_execute(ast::Node* node) {
    if (!node) return 0;

    if (node->kind == ast::NodeKind::FuncCall) {
        auto* fc = static_cast<ast::FuncCallNode*>(node);
        std::string cache_key = fc->name + "#" + std::to_string(fc->args.size());
        auto it = inline_caches_.find(cache_key);
        if (it != inline_caches_.end() && it->second.cached_target >= 0) {
            return it->second.cached_target;
        }
    }

    return tier1_execute(node);
}

int HybridRuntime::tier3_execute(ast::Node* node) {
    return tier2_execute(node);
}

int HybridRuntime::compile_function_ir(ast::FuncDefNode* func) {
    ir_.clear();
    int entry_point = static_cast<int>(ir_.size());

    if (func->body) {
        std::function<void(ast::Node*)> emit;
        emit = [&](ast::Node* n) {
            if (!n) return;
            if (n->kind == ast::NodeKind::Literal) {
                auto* lit = static_cast<ast::LiteralNode*>(n);
                std::string ckey = "c_" + lit->value;
                int cid;
                auto cit = constants_.find(ckey);
                if (cit != constants_.end()) {
                    cid = cit->second;
                } else {
                    cid = next_const_id_++;
                    constants_[ckey] = cid;
                }
                ir_.push_back({BytecodeIR::LOAD_CONST, cid, 0, 0, lit->value});
            } else if (n->kind == ast::NodeKind::BinaryExpr) {
                auto* bin = static_cast<ast::BinaryExprNode*>(n);
                emit(bin->left.get());
                emit(bin->right.get());
                ir_.push_back({BytecodeIR::BIN_OP, static_cast<int>(bin->op), 0, 0, ""});
            } else if (n->kind == ast::NodeKind::FuncCall) {
                auto* fc = static_cast<ast::FuncCallNode*>(n);
                for (auto& arg : fc->args) emit(arg.get());
                ir_.push_back({BytecodeIR::CALL, static_cast<int>(fc->args.size()), 0, 0, fc->name});
            }
        };
        emit(func->body.get());
    }

    ir_.push_back({BytecodeIR::RET, 0, 0, 0, ""});
    return entry_point;
}

int HybridRuntime::execute_ir(const std::vector<BytecodeIR>& code, int entry) {
    int pc = entry;
    int stack[64];
    int sp = 0;
    int result = 0;

    std::unordered_map<std::string, int> locals;

    while (pc < (int)code.size()) {
        const auto& instr = code[pc++];
        switch (instr.op) {
            case BytecodeIR::LOAD_CONST:
                stack[sp++] = std::stoi(instr.str_arg);
                break;
            case BytecodeIR::LOAD_VAR:
                stack[sp++] = locals[instr.str_arg];
                break;
            case BytecodeIR::STORE_VAR:
                locals[instr.str_arg] = stack[--sp];
                break;
            case BytecodeIR::BIN_OP: {
                int b = stack[--sp];
                int a = stack[--sp];
                int r = 0;
                switch (instr.arg1) {
                    case 0: r = a + b; break;
                    case 1: r = a - b; break;
                    case 2: r = a * b; break;
                    case 3: r = (b != 0) ? (a / b) : 0; break;
                    case 4: r = (b != 0) ? (a % b) : 0; break;
                    case 5: r = (a < b) ? 1 : 0; break;
                    case 6: r = (a > b) ? 1 : 0; break;
                    case 7: r = (a <= b) ? 1 : 0; break;
                    case 8: r = (a >= b) ? 1 : 0; break;
                    case 9: r = (a == b) ? 1 : 0; break;
                    case 10: r = (a != b) ? 1 : 0; break;
                    default: r = a; break;
                }
                stack[sp++] = r;
                break;
            }
            case BytecodeIR::CALL: {
                std::string fname = instr.str_arg;
                int argc = instr.arg1;
                int call_result = 0;

                if (fname == "print") {
                    for (int i = 0; i < argc && sp > 0; ++i) {
                        int val = stack[--sp];
                        if (i > 0) std::cout << " ";
                        std::cout << val;
                    }
                    std::cout << "\n";
                    call_result = 0;
                } else if (fname == "sqrt") {
                    int arg = (sp > 0) ? stack[--sp] : 0;
                    call_result = static_cast<int>(std::sqrt(arg));
                } else {
                    call_result = 0;
                }
                result = call_result;
                break;
            }
            case BytecodeIR::RET:
                return result;
            default:
                break;
        }
    }
    return result;
}

int HybridRuntime::execute_ir_optimized(const std::vector<BytecodeIR>& code, int entry) {
    int pc = entry;
    int stack[64];
    int sp = 0;
    int result = 0;

    std::unordered_map<std::string, int> locals;
    std::unordered_set<int> hot_pcs;

    for (const auto& [node, entry] : profile_table_) {
        if (entry.call_count >= promotion_threshold_calls_) {
            node->hot_path = true;
        }
    }

    while (pc < (int)code.size()) {
        if (hot_pcs.count(pc)) {
            pc++;
            continue;
        }

        const auto& instr = code[pc++];
        switch (instr.op) {
            case BytecodeIR::LOAD_CONST:
                stack[sp++] = std::stoi(instr.str_arg);
                break;
            case BytecodeIR::LOAD_VAR:
                stack[sp++] = locals[instr.str_arg];
                break;
            case BytecodeIR::STORE_VAR:
                locals[instr.str_arg] = stack[--sp];
                break;
            case BytecodeIR::BIN_OP: {
                int b = stack[--sp];
                int a = stack[--sp];
                int r = 0;
                switch (instr.arg1) {
                    case 0: r = a + b; break;
                    case 1: r = a - b; break;
                    case 2: r = a * b; break;
                    case 3: r = (b != 0) ? (a / b) : 0; break;
                    case 4: r = (b != 0) ? (a % b) : 0; break;
                    case 5: r = (a < b) ? 1 : 0; break;
                    case 6: r = (a > b) ? 1 : 0; break;
                    case 7: r = (a <= b) ? 1 : 0; break;
                    case 8: r = (a >= b) ? 1 : 0; break;
                    case 9: r = (a == b) ? 1 : 0; break;
                    case 10: r = (a != b) ? 1 : 0; break;
                    default: r = a; break;
                }
                stack[sp++] = r;
                break;
            }
            case BytecodeIR::CALL: {
                std::string fname = instr.str_arg;
                int argc = instr.arg1;
                int call_result = 0;
                if (fname == "print") {
                    for (int i = 0; i < argc && sp > 0; ++i) {
                        int val = stack[--sp];
                        if (i > 0) std::cout << " ";
                        std::cout << val;
                    }
                    std::cout << "\n";
                    call_result = 0;
                } else if (fname == "sqrt") {
                    int arg = (sp > 0) ? stack[--sp] : 0;
                    call_result = static_cast<int>(std::sqrt(arg));
                } else {
                    call_result = 0;
                }
                result = call_result;
                break;
            }
            case BytecodeIR::RET:
                return result;
            default:
                break;
        }
    }
    return result;
}

void HybridRuntime::record_execution(ast::Node* node, int duration_us) {
    ProfileEntry& entry = profile_table_[node];
    entry.execution_time_us += duration_us;
    entry.call_count++;

    if (should_promote(node)) {
        node->hot_path = true;
        if (global_tier_ < ExecutionTier::Tier2_JIT_Optimized) {
            global_tier_ = static_cast<ExecutionTier>(
                static_cast<int>(global_tier_) + 1);
            std::cout << "[HybridOpt] Promoted to Tier "
                      << static_cast<int>(global_tier_)
                      << " after " << entry.call_count << " calls ("
                      << entry.execution_time_us << "us total)\n";
        }
    }
}

bool HybridRuntime::should_promote(ast::Node* node) const {
    auto it = profile_table_.find(const_cast<ast::Node*>(node));
    if (it == profile_table_.end()) return false;
    return it->second.call_count >= promotion_threshold_calls_ &&
           it->second.execution_time_us >= promotion_threshold_us_;
}

ExecutionTier HybridRuntime::promote_to_jit(ast::Node* node) {
    if (node) {
        ProfileEntry& entry = profile_table_[node];
        entry.current_tier = global_tier_;
        node->hot_path = true;
    }
    return global_tier_;
}

ExecutionTier HybridRuntime::get_current_tier(ast::Node* node) const {
    auto it = profile_table_.find(const_cast<ast::Node*>(node));
    if (it != profile_table_.end()) {
        return it->second.current_tier;
    }
    return global_tier_;
}

void HybridRuntime::profile_and_promote() {
    for (auto& [node, entry] : profile_table_) {
        if (!node->hot_path && entry.call_count >= promotion_threshold_calls_) {
            promote_to_jit(node);
            if (node->kind == ast::NodeKind::FuncDef) {
                auto* fd = static_cast<ast::FuncDefNode*>(node);
                compile_function_ir(fd);
            }
        }
    }
}

}
