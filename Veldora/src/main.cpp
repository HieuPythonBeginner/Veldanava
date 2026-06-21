#include "frontend/parser/parser.h"
#include "backend/interpreter/interpreter.h"
#include "backend/hybrid/hybrid.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: Veldora <file.veldora>\n";
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
        veldora::HybridRuntime runtime;
        runtime.load_cache("veldora.cache");
        int result = runtime.eval(src);

        if (result != 0) return result;

        runtime.profile_and_jit();

        if (result == 0) {
            runtime.save_cache("veldora.cache");
        }

        std::cout << "[OK] Executed " << argv[1] << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
