#include <iostream>
#include "semantic.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    runParser(argv[1]);

    if (!runSemanticAnalysis()) {
        return 1;
    }

    return 0;
}