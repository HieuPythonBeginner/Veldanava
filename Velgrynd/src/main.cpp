#include "frontend/parser/parser.h"
#include "backend/jit/jit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: velgrynd_JIT <file.velgrynd>\n";
        return 1;
    }
    std::string filepath = argv[1];
    size_t dot = filepath.find_last_of('.');
    if (dot == std::string::npos || filepath.substr(dot) != ".velgrynd") {
        std::cerr << "Error: Velgrynd requires .velgrynd extension\n";
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
        std::vector<int> bytecode = jit::compile(program.get());
        std::cout << "[OK] JIT compiled " << argv[1] << " (" << bytecode.size() << " bytes)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
