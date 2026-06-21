#include "frontend/parser/parser.h"
#include "backend/interpreter/interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: velzard_Interpreter <file.velzard>\n";
        return 1;
    }
    std::string filepath = argv[1];
    size_t dot = filepath.find_last_of('.');
    if (dot == std::string::npos || filepath.substr(dot) != ".velzard") {
        std::cerr << "Error: Velzard requires .velzard extension\n";
        return 1;
    }

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
        int result = interpreter::eval(program.get());
        std::cout << "[OK] Interpreted " << argv[1] << " (result=" << result << ")\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
