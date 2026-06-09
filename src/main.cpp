#include "frontend/parser/parser.h"
#include "frontend/lexer/lexer.h"
#include "backend/codegen/codegen.h"
#include "backend/vm/vm.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

bool run_test(const string& name, const string& src, bool expect_success = true) {
    try {
        auto program = parser::parse_source(src);
        codegen::CodeGenerator gen;
        auto bytecode = gen.generate(program.get());
        vm::VirtualMachine vm;
        vm.set_strings(gen.get_string_pool());
        vm.load(bytecode);
        vm.run();
        cout << "[PASS] " << name << "\n";
        return true;
    } catch (const exception& e) {
        if (!expect_success) {
            cout << "[PASS] " << name << " - expected error: " << e.what() << "\n";
            return true;
        }
        cout << "[FAIL] " << name << " - " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char** argv) {
    if (argc > 1) {
        ifstream file(argv[1]);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << argv[1] << "\n";
            return 1;
        }
        stringstream buffer;
        buffer << file.rdbuf();
        string src = buffer.str();

        try {
            auto program = parser::parse_source(src);
            codegen::CodeGenerator gen;
            auto bytecode = gen.generate(program.get());
            vm::VirtualMachine vm;
            vm.set_strings(gen.get_string_pool());
            vm.load(bytecode);
            vm.run();
            cout << "[OK] Executed " << argv[1] << "\n";
            return 0;
        } catch (const exception& e) {
            cerr << "[ERROR] " << e.what() << "\n";
            return 1;
        }
    }

    int passed = 0, total = 0;

    cout << "=== Veldanava Test Suite ===\n\n";

    total++; if (run_test("simple let", "let i32 x = 42;")) passed++;
    total++; if (run_test("binary add", "let i32 x = 1 + 2;")) passed++;
    total++; if (run_test("binary mult", "let i32 x = 3 * 4;")) passed++;
    total++; if (run_test("function no params", "func main() -> i32:\n    let i32 x = 0;\n    return x;\n;\n")) passed++;
    total++; if (run_test("function with params", "func add(a: i32, b: i32) -> i32:\n    let i32 x = a + b;\n    return x;\n;\n")) passed++;
    total++; if (run_test("if statement", "if true:\n    let i32 y = 1;\n;\n")) passed++;
    total++; if (run_test("while statement", "while true:\n    let i32 x = 1;\n;\n")) passed++;
    total++; if (run_test("for statement", "for i in range(3):\n    let i32 x = 1;\n;\n")) passed++;

    cout << "\n=== Results: " << passed << "/" << total << " passed ===\n";

    return passed == total ? 0 : 1;
}
