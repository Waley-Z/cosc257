#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <stdbool.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>

#define prt(x)             \
    if (x) {               \
        printf("%s\n", x); \
    }

/* This function reads the given llvm file and loads the LLVM IR into
         data-structures that we can works on for optimization phase.
*/

LLVMModuleRef createLLVMModel(char* filename) {
    char* err = 0;

    LLVMMemoryBufferRef ll_f = 0;
    LLVMModuleRef m = 0;

    LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

    if (err != NULL) {
        prt(err);
        return NULL;
    }

    LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

    if (err != NULL) {
        prt(err);
    }

    return m;
}

/* Checks if the instruction instB can be safely replaced by instA. */
bool isSafeToReplace(LLVMValueRef instA, LLVMValueRef instB) {
    if (!LLVMIsALoadInst(instA) && !LLVMIsALoadInst(instB)) {
        return true;
    }

    LLVMValueRef instIter = LLVMGetNextInstruction(instA);
    while (instIter != instB) {
        if (LLVMIsAStoreInst(instIter) && LLVMGetOperand(instIter, 1) == LLVMGetOperand(instA, 0)) {
            return false;
        }
        instIter = LLVMGetNextInstruction(instIter);
    }

    return true;
}

/* Removes the dead code from the basic block. */
void deadCodeElimination(LLVMBasicBlockRef bb) {
    printf("Dead code elimination...\n");
    LLVMValueRef instIter = LLVMGetFirstInstruction(bb);
    while (instIter) {
        LLVMValueRef nextInst = LLVMGetNextInstruction(instIter);
        if (!LLVMIsAStoreInst(instIter) &&
            !LLVMIsATerminatorInst(instIter) &&
            !LLVMIsACallInst(instIter) &&
            !LLVMIsAAllocaInst(instIter) &&
            !LLVMGetFirstUse(instIter)) {
            printf("Removing dead code: ");
            LLVMDumpValue(instIter);
            printf("\n");
            LLVMInstructionEraseFromParent(instIter);
        }
        instIter = nextInst;
    }
}

/* Performs constant folding on the basic block. */
void constantFolding(LLVMBasicBlockRef bb) {
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
         inst = LLVMGetNextInstruction(inst)) {
        LLVMOpcode op = LLVMGetInstructionOpcode(inst);
        if ((op == LLVMAdd || op == LLVMSub || op == LLVMMul) &&
            LLVMIsConstant(LLVMGetOperand(inst, 0)) &&
            LLVMIsConstant(LLVMGetOperand(inst, 1))) {
            LLVMValueRef constOp1 = LLVMGetOperand(inst, 0);
            LLVMValueRef constOp2 = LLVMGetOperand(inst, 1);
            LLVMValueRef constResult = NULL;

            if (op == LLVMAdd) {
                constResult = LLVMConstAdd(constOp1, constOp2);
            } else if (op == LLVMSub) {
                constResult = LLVMConstSub(constOp1, constOp2);
            } else if (op == LLVMMul) {
                constResult = LLVMConstMul(constOp1, constOp2);
            }

            if (constResult != NULL) {
                LLVMReplaceAllUsesWith(inst, constResult);
            }
        }
    }
}

void walkBBInstructions(LLVMBasicBlockRef bb) {
    for (LLVMValueRef instA = LLVMGetFirstInstruction(bb); instA;
         instA = LLVMGetNextInstruction(instA)) {
        for (LLVMValueRef instB = LLVMGetNextInstruction(instA); instB;
             instB = LLVMGetNextInstruction(instB)) {
            if (LLVMGetInstructionOpcode(instA) == LLVMGetInstructionOpcode(instB) &&
                LLVMGetOperand(instA, 0) == LLVMGetOperand(instB, 0) &&
                LLVMGetOperand(instA, 1) == LLVMGetOperand(instB, 1) &&
                LLVMGetInstructionOpcode(instA) != LLVMAlloca &&
                isSafeToReplace(instA, instB)) {
                printf("Detected common subexpression\n");
                LLVMDumpValue(instA);
                LLVMDumpValue(instB);
                LLVMReplaceAllUsesWith(instB, instA);
            }
        }
    }
}

void walkBasicblocks(LLVMValueRef function) {
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
         basicBlock;
         basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
        printf("In basic block\n");
        walkBBInstructions(basicBlock);
        constantFolding(basicBlock);
        deadCodeElimination(basicBlock);
    }
}

void walkFunctions(LLVMModuleRef module) {
    for (LLVMValueRef function = LLVMGetFirstFunction(module);
         function;
         function = LLVMGetNextFunction(function)) {
        const char* funcName = LLVMGetValueName(function);

        printf("Function Name: %s\n", funcName);

        walkBasicblocks(function);
    }
}

void walkGlobalValues(LLVMModuleRef module) {
    for (LLVMValueRef gVal = LLVMGetFirstGlobal(module);
         gVal;
         gVal = LLVMGetNextGlobal(gVal)) {
        const char* gName = LLVMGetValueName(gVal);
        printf("Global variable name: %s\n", gName);
    }
}

int main(int argc, char** argv) {
    LLVMModuleRef m;

    if (argc == 2) {
        m = createLLVMModel(argv[1]);
    } else {
        m = NULL;
        return 1;
    }

    if (m != NULL) {
        // LLVMDumpModule(m);
        walkGlobalValues(m);
        walkFunctions(m);
        LLVMPrintModuleToFile(m, "test_new.ll", NULL);
    } else {
        fprintf(stderr, "m is NULL\n");
    }

    return 0;
}
