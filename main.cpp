#include <iostream>
#include "part1/semantic.h"
#include "part3/llvm_parser.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <testfile>.c <testfile>.ll" << endl;
        return 1;
    }

    char* cFile = argv[1];
    char* llFile = argv[2];

    if (!runParser(cFile)) {
        cerr << "Parsing failed." << endl;
        return 1;
    }

    if (!runSemanticAnalysis()) {
        cerr << "Semantic analysis failed." << endl;
        return 1;
    }

    llvm_parse(llFile, "test_new.ll");
    return 0;
}