#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "frontend/lexer/lexer.h"
#include "frontend/parser/parser.h"
#include "backend/interpreter/interpreter.h"
#include <vm_instruction.h>

namespace veldora {

struct ProfileStats {
    int call_count = 0;
    int total_time = 0;
};

class HybridRuntime {
public:
    HybridRuntime();
    ~HybridRuntime();

    int eval(const std::string& source);
    void profile_and_jit();
    bool load_cache(const std::string& path);
    bool save_cache(const std::string& path);

    int get_last_instruction_count() const { return last_instruction_count_; }

private:
    std::vector<Token> cached_tokens_;
    std::unique_ptr<ProgramNode> cached_program_;
    std::vector<vm::Instruction> cached_bytecode_;
    std::vector<std::string> cached_strings_;
    std::unordered_map<std::string, ProfileStats> hot_paths_;
    bool use_interpreter_ = true;
    bool use_jit_ = false;
    bool use_aot_ = false;
    int last_instruction_count_ = 0;
    std::string last_source_hash_;
};

}
