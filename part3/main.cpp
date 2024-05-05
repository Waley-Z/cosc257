#include <iostream>
#include "llvm_parser.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <testfile>.ll" << std::endl;
    }
    
    llvm_parse(argv[1], "test_new.ll");
}