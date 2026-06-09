#include "frontend/parser/parser.h"
#include "frontend/lexer/lexer.h"
#include "backend/codegen/codegen.h"
#include "backend/vm/vm.h"
#include <iostream>
#include <fstream>
#include <sstream>

bool run_test(const std::string& name, const std::string& src, bool expect_success = true) {
    try {
        auto program = parser::parse_source(src);
        auto bytecode = codegen::generate_code(program.get());
        std::cout << "[PASS] " << name << " - generated " << bytecode.size() << " instructions\n";
        return true;
    } catch (const std::exception& e) {
        if (!expect_success) {
            std::cout << "[PASS] " << name << " - expected error: " << e.what() << "\n";
            return true;
        }
        std::cout << "[FAIL] " << name << " - " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char** argv) {
    // If file argument provided, run file; else run tests
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << argv[1] << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string src = buffer.str();

        try {
            auto program = parser::parse_source(src);
            auto bytecode = codegen::generate_code(program.get());
            vm::VirtualMachine vm;
            vm.load(bytecode);
            vm.run();
            std::cout << "[OK] Executed " << argv[1] << "\n";
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] " << e.what() << "\n";
            return 1;
        }
    }

    int passed = 0, total = 0;

    std::cout << "=== Veldanava Test Suite ===\n\n";

    total++; if (run_test("simple let", "let x: i32 = 42;")) passed++;
    total++; if (run_test("binary add", "let x: i32 = 1 + 2;")) passed++;
    total++; if (run_test("binary mult", "let x: i32 = 3 * 4;")) passed++;
    total++; if (run_test("function no params", "func main() -> i32:\n    let x: i32 = 0;\n    return x;\n;\n")) passed++;
    total++; if (run_test("function with params", "func add(a: i32, b: i32) -> i32:\n    let x: i32 = a + b;\n    return x;\n;\n")) passed++;
    total++; if (run_test("if statement", "if true:\n    let y: i32 = 1;\n;\n")) passed++;
    total++; if (run_test("while statement", "while true:\n    let x: i32 = 1;\n;\n")) passed++;
    total++; if (run_test("for statement", "for i in range(10):\n    let x: i32 = i;\n;\n")) passed++;

    std::cout << "\n=== Results: " << passed << "/" << total << " passed ===\n";
    return passed == total ? 0 : 1;
}