#include "frontend/parser/parser.h"
#include "frontend/lexer/lexer.h"
#include "backend/codegen/codegen.h"
#include "backend/vm/vm.h"
#include "middle/ownership/ownership.h"
#include "middle/typechecker/typechecker.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>

using namespace std;

static void debug_print_tokens(const std::string& src) {
    auto tokens = lexer::tokenize(src);
    for (const auto& t : tokens) {
        cout << (int)t.type << "\t" << t.lexeme << "\t" << t.line << ":" << t.col << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc > 1) {
        std::string filepath = argv[1];
        size_t dot = filepath.find_last_of('.');
        if (dot == std::string::npos || filepath.substr(dot) != ".veldanava") {
            std::cerr << "Error: veldanc requires .veldanava extension\n";
            return 1;
        }

        ifstream file(argv[1]);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << argv[1] << "\n";
            return 1;
        }
        stringstream buffer;
        buffer << file.rdbuf();
        string src = buffer.str();

        std::string first_line = src.substr(0, src.find('\n'));
        bool has_pre_descent = (first_line.find("#Pre_Descent") != std::string::npos);
        bool has_primordial = (first_line.find("Primordial_Regalia") != std::string::npos);

        if (has_pre_descent && has_primordial) {
            // AOT mode - proceed normally
        } else if (!has_pre_descent && !has_primordial) {
            std::string veldora_path = "/home/dr-bright-rathalus/Veldanava/build-veldora/Veldora_Hybrid_AOT_JIT_Interpreter";
            std::cerr << "[Routing] No magic header — switching to Veldora Hybrid mode\n";
            execl(veldora_path.c_str(), veldora_path.c_str(), argv[1], nullptr);
            std::cerr << "[ERROR] Failed to launch Veldora Hybrid\n";
            return 1;
        } else if (has_pre_descent && !has_primordial) {
            std::cerr << "[ERROR] Missing 'Primordial_Regalia' in magic header on line 1\n";
            std::cerr << "[HINT] Correct syntax: #Pre_Descent Primordial_Regalia\n";
            std::cerr << "[HINT] To use veldanc compiler, you must type the exact magic header on line 1\n";
            return 1;
        } else if (!has_pre_descent && has_primordial) {
            std::cerr << "[ERROR] Missing '#Pre_Descent' in magic header on line 1\n";
            std::cerr << "[HINT] Correct syntax: #Pre_Descent Primordial_Regalia\n";
            std::cerr << "[HINT] To use veldanc compiler, you must type the exact magic header on line 1\n";
            return 1;
        }

        try {
            auto program = parser::parse_source(src);
            parser::validate_sanctions(program.get());
            ownership::init();
            ownership::validate_ast(program.get());
            
            auto type_errors = typechecker::type_check(program.get());
            for (const auto& err : type_errors) {
                std::cerr << "[TYPE ERROR] " << err << "\n";
            }
            if (!type_errors.empty()) {
                return 1;
            }
            
            codegen::CodeGenerator gen;
            auto bytecode = gen.generate(program.get());
            vm::VirtualMachine vm;
            vm.set_strings(gen.get_string_pool());
            
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
            
            vm.load(bytecode);
            vm.run(100);
            cout << "[OK] Executed " << argv[1] << "\n";
            return 0;
        } catch (const exception& e) {
            cerr << "[ERROR] " << e.what() << "\n";
            return 1;
        }
    }

    cout << "Usage: veldanc <file.veldanava>\n";
    return 1;
}
