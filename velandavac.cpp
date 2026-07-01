#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace std;

static void print_usage() {
    cout << "running_true_dragon_core - Veldanava ecosystem compiler driver\n";
    cout << "Usage: running_true_dragon_core <file>\n\n";
    cout << "Extensions:\n";
    cout << "  .veldanava  -> veldanc (Veldanava Compiler / Hybrid)\n";
    cout << "  .veldora    -> Veldora Hybrid (AOT/JIT/Interpreter)\n";
    cout << "  .velzard    -> Velzard Interpreter\n";
    cout << "  .velgrynd   -> Velgrynd JIT\n";
}

static bool route_by_extension(const string& filepath, const string& ext) {
    string bin;
    string mode;

    if (ext == ".veldanava") {
        bin = "/home/dr-bright-rathalus/Veldanava/build/veldanc";
        mode = "Veldanava Compiler";
    } else if (ext == ".veldora") {
        bin = "/home/dr-bright-rathalus/Veldanava/build-veldora/Veldora_Hybrid_AOT_JIT_Interpreter";
        mode = "Veldora Hybrid";
    } else if (ext == ".velzard") {
        bin = "/home/dr-bright-rathalus/Veldanava/build-velzard/velzard_Interpreter";
        mode = "Velzard Interpreter";
    } else if (ext == ".velgrynd") {
        bin = "/home/dr-bright-rathalus/Veldanava/build-velgrynd/velgrynd_JIT";
        mode = "Velgrynd JIT";
    } else {
        return false;
    }

    cout << "[Routing] " << filepath << " (" << ext << ") -> " << mode << "\n";
    execl(bin.c_str(), bin.c_str(), filepath.c_str(), nullptr);
    cerr << "[ERROR] Failed to launch " << mode << " at " << bin << "\n";
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    string filepath = argv[1];
    size_t dot = filepath.find_last_of('.');
    if (dot == string::npos) {
        cerr << "Error: Unknown file type (no extension)\n";
        return 1;
    }

    string ext = filepath.substr(dot);

    if (!route_by_extension(filepath, ext)) {
        cerr << "Error: Unsupported extension '" << ext << "'\n";
        cerr << "Supported: .veldanava .veldora .velzard .velgrynd\n";
        return 1;
    }

    return 0;
}
