#ifndef ASSEMBLY_GENERATOR_H
#define ASSEMBLY_GENERATOR_H

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

#include <map>
#include <string>
#include <vector>

class AssemblyGenerator {
   public:
    AssemblyGenerator(const char* inputFilename, const char* outputFilename);
    void generateAssembly();

   private:
    void generateInstIndexMap(LLVMBasicBlockRef bb);
    void computeLiveness(LLVMBasicBlockRef bb);
    int countNumUses(LLVMValueRef value);
    bool compareUses(LLVMValueRef a, LLVMValueRef b);
    void regAllocation(LLVMBasicBlockRef bb);
    void createBBLabels(LLVMValueRef function);
    void printDirectives(LLVMValueRef function, int offset);
    int getOffsetMap(LLVMValueRef function);
    void generateFunctionCode(LLVMValueRef function);
    void generateBasicBlockCode(LLVMBasicBlockRef bb);
    void generateInstructionCode(LLVMValueRef inst);
    void generateReturnCode(LLVMValueRef inst);
    void generateLoadCode(LLVMValueRef inst);
    void generateStoreCode(LLVMValueRef inst);
    void generateCallCode(LLVMValueRef inst);
    void generateBranchCode(LLVMValueRef inst);
    void generateArithmeticCode(LLVMValueRef inst);
    void codeGeneration();
    void walkBasicBlocks(LLVMValueRef function);
    void walkFunctionsAssembly();

    static const int NUM_REGS = 3;
    static const char* REGS[NUM_REGS];

    std::map<LLVMValueRef, int> instIndex;
    std::map<LLVMValueRef, std::pair<int, int>> liveRange;
    std::map<LLVMValueRef, const char*> regMap;
    std::map<LLVMBasicBlockRef, std::string> bbLabels;
    std::map<LLVMValueRef, int> offsetMap;

    LLVMModuleRef module;
    const char* outputFilename;
};

#endif  // ASSEMBLY_GENERATOR_H