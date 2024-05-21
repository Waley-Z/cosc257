#include <iostream>

#include "part1/semantic.h"
#include "part2/ir_builder.h"
#include "part3/llvm_parser.h"
#include "part4/assembly_generator.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <testfile>.c" << endl;
        return 1;
    }

    char* cFile = argv[1];

    // Part 1
    if (!runParser(cFile)) {
        cerr << "Parsing failed." << endl;
        return 1;
    }

    if (!runSemanticAnalysis(false)) {
        cerr << "Semantic analysis failed." << endl;
        return 1;
    }

    // Part 2
    if (!runIRBuilder("out.ll")) {
        cerr << "IR builder failed." << endl;
        return 1;
    }

    // Part 3
    llvm_parse("out.ll", "out_new.ll");

    // Part 4
    AssemblyGenerator("out_new.ll", "out_new.s").generateAssembly();

    return 0;
}