#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include <llvm-c/Core.h>

#include <string>
#include <unordered_map>

#include "../part1/ast.h"

class IRBuilder {
   public:
    LLVMModuleRef buildIR();

   private:
    LLVMModuleRef module;
    LLVMBuilderRef builder;

    void buildFunction(astNode* func_node);
    LLVMBasicBlockRef buildStatement(astNode* stmt_node, std::unordered_map<std::string, LLVMValueRef>& var_map, LLVMValueRef retAlloca);
    LLVMValueRef buildExpression(astNode* expr_node, std::unordered_map<std::string, LLVMValueRef>& var_map);
    void removeUnusedBasicBlocks(LLVMValueRef func);
};

void printLLVMIR(LLVMModuleRef module);
bool runIRBuilder(const string &filename);

#endif  // IR_BUILDER_H
