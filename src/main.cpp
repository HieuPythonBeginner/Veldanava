#include "frontend/parser/parser.h"
#include "frontend/lexer/lexer.h"
#include "backend/codegen/codegen.h"
#include "backend/vm/vm.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

static void debug_print_tokens(const std::string& src) {
    auto tokens = lexer::tokenize(src);
    for (const auto& t : tokens) {
        cout << (int)t.type << "\t" << t.lexeme << "\t" << t.line << ":" << t.col << "\n";
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
            parser::validate_sanctions(program.get());



            codegen::CodeGenerator gen;
            auto bytecode = gen.generate(program.get());
            vm::VirtualMachine vm;
            vm.set_strings(gen.get_string_pool());
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
