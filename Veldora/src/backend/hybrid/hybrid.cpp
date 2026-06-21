#include "hybrid.h"
#include "frontend/parser/parser.h"
#include "frontend/lexer/lexer.h"
#include "backend/bridge/bridge.h"
#include <vm_api.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <cmath>
#include <cstdint>

namespace veldora {

static size_t simple_hash(const std::string& s) {
    size_t h = 0;
    for (char c : s) h = h * 31 + static_cast<size_t>(c);
    return h;
}

static void register_builtins(vm::VirtualMachine& vm) {
    vm.register_function("print", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        for (int i = 0; i < argc; i++) {
            int val = argv[i];
            if (val >= 1000 && val - 1000 < (int)v->get_string_pool().size()) {
                std::cout << v->get_string_pool()[val - 1000];
            } else {
                std::cout << val;
            }
            if (i < argc - 1) std::cout << " ";
        }
        std::cout << "\n";
        return 0;
    }, 1);

    vm.register_function("range", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? argv[0] : 0;
    }, 1);

    vm.register_function("sqrt", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? (int)std::sqrt(argv[0]) : 0;
    }, 1);

    vm.register_function("sin", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? (int)std::sin(argv[0]) : 0;
    }, 1);

    vm.register_function("cos", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? (int)std::cos(argv[0]) : 0;
    }, 1);

    vm.register_function("tan", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? (int)std::tan(argv[0]) : 0;
    }, 1);

    vm.register_function("abs", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? std::abs(argv[0]) : 0;
    }, 1);

    vm.register_function("floor", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? (int)std::floor(argv[0]) : 0;
    }, 1);

    vm.register_function("ceil", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? (int)std::ceil(argv[0]) : 0;
    }, 1);

    vm.register_function("pow", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        if (argc >= 2) return (int)std::pow(argv[0], argv[1]);
        return 0;
    }, 2);

    vm.register_function("double", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        return argc > 0 ? argv[0] * 2 : 0;
    }, 1);

    vm.register_function("scribe", [](vm::VirtualMachine* v, int argc, const int* argv) -> int {
        for (int i = 0; i < argc; i++) {
            int val = argv[i];
            if (val >= 1000 && val - 1000 < (int)v->get_string_pool().size()) {
                std::cout << v->get_string_pool()[val - 1000];
            } else {
                std::cout << val;
            }
            if (i < argc - 1) std::cout << " ";
        }
        return 0;
    }, 1);
}

HybridRuntime::HybridRuntime() = default;
HybridRuntime::~HybridRuntime() = default;

int HybridRuntime::eval(const std::string& source) {
    if (use_aot_ && !cached_bytecode_.empty() && simple_hash(source) == simple_hash(last_source_hash_)) {
        std::cout << "[Veldora Hybrid] Tier 3: AOT cache hit, skipping lexer\n";
        vm::VirtualMachine vm;
        register_builtins(vm);
        vm.load(cached_bytecode_);
        vm.set_strings(cached_strings_);
        vm.run(1000000);
        return 0;
    }

    use_interpreter_ = true;
    use_jit_ = false;
    use_aot_ = false;
    hot_paths_.clear();
    last_source_hash_ = source;

    std::cout << "[Veldora Hybrid] Tier 1: Interpreter (cold start)\n";

    try {
        auto tokens = tokenize(source);
        cached_tokens_ = std::move(tokens);

        auto program = parse_tokens(cached_tokens_);
        cached_program_ = std::move(program);

        BridgeCompiler compiler;
        std::vector<std::string> strings;
        cached_bytecode_ = compiler.compile(cached_program_.get(), strings);
        cached_strings_ = strings;

        vm::VirtualMachine vm;
        register_builtins(vm);
        vm.load(cached_bytecode_);
        vm.set_strings(cached_strings_);
        vm.run(1000000);
        last_instruction_count_++;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}

void HybridRuntime::profile_and_jit() {
    if (cached_bytecode_.empty()) {
        std::cout << "[Veldora Hybrid] Tier 2: No bytecode cached, skipping JIT\n";
        return;
    }

    std::cout << "[Veldora Hybrid] Tier 2: JIT caching " << cached_bytecode_.size() << " instructions\n";

    hot_paths_["__main__"] = {1, static_cast<int>(cached_bytecode_.size())};

    use_interpreter_ = false;
    use_jit_ = true;
    use_aot_ = false;
    std::cout << "[Veldora Hybrid] Promoted bytecode to JIT cache\n";
}

bool HybridRuntime::load_cache(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    uint32_t instr_count = 0;
    f.read(reinterpret_cast<char*>(&instr_count), sizeof(instr_count));
    if (!f) return false;

    cached_bytecode_.clear();
    cached_bytecode_.reserve(instr_count);
    for (uint32_t i = 0; i < instr_count; i++) {
        vm::Instruction instr(vm::Opcode::NOP);
        f.read(reinterpret_cast<char*>(&instr.opcode), sizeof(instr.opcode));
        f.read(reinterpret_cast<char*>(&instr.arg1), sizeof(instr.arg1));
        f.read(reinterpret_cast<char*>(&instr.arg2), sizeof(instr.arg2));
        f.read(reinterpret_cast<char*>(&instr.arg3), sizeof(instr.arg3));
        if (!f) return false;
        cached_bytecode_.push_back(instr);
    }

    uint32_t string_count = 0;
    f.read(reinterpret_cast<char*>(&string_count), sizeof(string_count));
    if (!f) return false;

    cached_strings_.clear();
    cached_strings_.reserve(string_count);
    for (uint32_t i = 0; i < string_count; i++) {
        uint32_t len = 0;
        f.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!f) return false;
        std::string str(len, '\0');
        f.read(&str[0], len);
        if (!f) return false;
        cached_strings_.push_back(std::move(str));
    }

    uint32_t hash_len = 0;
    f.read(reinterpret_cast<char*>(&hash_len), sizeof(hash_len));
    if (!f) return false;
    last_source_hash_.resize(hash_len);
    f.read(&last_source_hash_[0], hash_len);
    if (!f) return false;

    std::cout << "[Veldora Hybrid] Tier 3: AOT cache loaded from " << path << "\n";
    use_interpreter_ = false;
    use_jit_ = false;
    use_aot_ = true;
    return true;
}

bool HybridRuntime::save_cache(const std::string& path) {
    if (cached_bytecode_.empty()) {
        std::cerr << "[Veldora Hybrid] No bytecode to cache\n";
        return false;
    }

    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    uint32_t instr_count = static_cast<uint32_t>(cached_bytecode_.size());
    f.write(reinterpret_cast<const char*>(&instr_count), sizeof(instr_count));
    for (const auto& instr : cached_bytecode_) {
        f.write(reinterpret_cast<const char*>(&instr.opcode), sizeof(instr.opcode));
        f.write(reinterpret_cast<const char*>(&instr.arg1), sizeof(instr.arg1));
        f.write(reinterpret_cast<const char*>(&instr.arg2), sizeof(instr.arg2));
        f.write(reinterpret_cast<const char*>(&instr.arg3), sizeof(instr.arg3));
    }

    uint32_t string_count = static_cast<uint32_t>(cached_strings_.size());
    f.write(reinterpret_cast<const char*>(&string_count), sizeof(string_count));
    for (const auto& str : cached_strings_) {
        uint32_t len = static_cast<uint32_t>(str.size());
        f.write(reinterpret_cast<const char*>(&len), sizeof(len));
        f.write(str.data(), len);
    }

    uint32_t hash_len = static_cast<uint32_t>(last_source_hash_.size());
    f.write(reinterpret_cast<const char*>(&hash_len), sizeof(hash_len));
    if (hash_len > 0) f.write(last_source_hash_.data(), hash_len);

    std::cout << "[Veldora Hybrid] AOT cache saved to " << path << "\n";
    return true;
}

}
