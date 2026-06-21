#include "frontend/parser/parser.h"
#include "backend/hybrid_opt/hybrid_opt.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: veldanc_hybrid_opt <file.veldanava>\n";
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
        hybrid_opt::HybridRuntime runtime;
        int result = runtime.execute(program.get());
        runtime.profile_and_promote();
        std::cout << "[OK] HybridOpt executed " << argv[1] << " (result=" << result << ")\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
