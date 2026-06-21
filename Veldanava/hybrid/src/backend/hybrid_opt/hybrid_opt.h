#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <functional>
#include <cmath>
#include "frontend/ast/ast.h"

namespace hybrid_opt {

enum class ExecutionTier {
    Tier0_Interpreter = 0,
    Tier1_JIT_Basic = 1,
    Tier2_JIT_Optimized = 2,
    Tier3_Max = 3
};

struct ProfileEntry {
    int call_count = 0;
    int execution_time_us = 0;
    ExecutionTier current_tier = ExecutionTier::Tier0_Interpreter;
};

struct InlineCache {
    std::string key;
    int cached_type = -1;
    int cached_target = -1;
    int hit_count = 0;
};

struct BytecodeIR {
    enum Op { LOAD_CONST, LOAD_VAR, STORE_VAR, BIN_OP, CALL, RET, JMP, JTRUE, JFALSE, PRINT } op;
    int arg1 = 0, arg2 = 0, arg3 = 0;
    std::string str_arg;
    BytecodeIR() = default;
    BytecodeIR(Op o, int a1=0, int a2=0, int a3=0, const std::string& s="")
        : op(o), arg1(a1), arg2(a2), arg3(a3), str_arg(s) {}
};

class HybridRuntime {
public:
    HybridRuntime();
    ~HybridRuntime();

    int execute(ast::ProgramNode* program);
    void profile_and_promote();
    ExecutionTier promote_to_jit(ast::Node* node);
    ExecutionTier get_current_tier(ast::Node* node) const;

private:
    ast::ProgramNode* program_;
    std::unordered_map<ast::Node*, ProfileEntry> profile_table_;
    std::unordered_map<std::string, InlineCache> inline_caches_;

    ExecutionTier global_tier_ = ExecutionTier::Tier0_Interpreter;
    int promotion_threshold_calls_ = 50;
    int promotion_threshold_us_ = 5000;

    std::vector<BytecodeIR> ir_;
    std::unordered_map<std::string, int> constants_;
    std::unordered_map<std::string, int> variables_;
    int next_const_id_ = 1000;

    int tier0_execute(ast::Node* node);
    int tier1_execute(ast::Node* node);
    int tier2_execute(ast::Node* node);
    int tier3_execute(ast::Node* node);

    void record_execution(ast::Node* node, int duration_us);
    bool should_promote(ast::Node* node) const;

    int compile_function_ir(ast::FuncDefNode* func);
    int execute_ir(const std::vector<BytecodeIR>& code, int entry);
    int execute_ir_optimized(const std::vector<BytecodeIR>& code, int entry);
};

}
