#include "llvm_parser.h"

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <stdbool.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

using namespace std;

typedef unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> BBValueSetMap;
typedef unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> BBPredMap;

#define prt(x)             \
    if (x) {               \
        printf("%s\n", x); \
    }

/* This function reads the given llvm file and loads the LLVM IR into
         data-structures that we can works on for optimization phase.
*/

LLVMModuleRef createLLVMModel(const char* filename) {
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
    printf("Dead code elimination:\n");
    LLVMValueRef instIter = LLVMGetFirstInstruction(bb);
    while (instIter) {
        LLVMValueRef nextInst = LLVMGetNextInstruction(instIter);
        if (!LLVMIsAStoreInst(instIter) &&
            !LLVMIsATerminatorInst(instIter) &&
            !LLVMIsACallInst(instIter) &&
            !LLVMIsAAllocaInst(instIter) &&
            !LLVMGetFirstUse(instIter)) {
            LLVMDumpValue(instIter);
            printf("\n");
            LLVMInstructionEraseFromParent(instIter);
        }
        instIter = nextInst;
    }
}

/* Performs constant folding on the basic block. */
void constantFolding(LLVMBasicBlockRef bb) {
    printf("Constant folding:\n");
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
         inst = LLVMGetNextInstruction(inst)) {
        LLVMOpcode op = LLVMGetInstructionOpcode(inst);
        if ((op == LLVMAdd || op == LLVMSub || op == LLVMMul) &&
            LLVMIsConstant(LLVMGetOperand(inst, 0)) &&
            LLVMIsConstant(LLVMGetOperand(inst, 1))) {
            LLVMDumpValue(inst);
            printf("\n");

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

/* Performs common subexpression elimination on the basic block. */
void commonSubexpressionElimination(LLVMBasicBlockRef bb) {
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
                printf("\n");
                LLVMDumpValue(instB);
                printf("\n");
                LLVMReplaceAllUsesWith(instB, instA);
            }
        }
    }
}

void localOptimizations(LLVMValueRef function) {
    printf("Local Optimizations\n");
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
         basicBlock;
         basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
        printf("In basic block\n");
        commonSubexpressionElimination(basicBlock);
        constantFolding(basicBlock);
        deadCodeElimination(basicBlock);
    }
}

/* Computes the GEN and KILL sets for each basic block. */
void computeGenKillSets(LLVMValueRef function, BBValueSetMap& genSets, BBValueSetMap& killSets) {
    // Compute the set "S" of all store instructions in the given function
    set<LLVMValueRef> storeSet;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
            if (LLVMIsAStoreInst(inst)) {
                storeSet.insert(inst);
            }
        }
    }

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        // Sets GEN and KILL are local to each basic block
        set<LLVMValueRef> bbGEN, bbKILL;
        for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
            if (LLVMIsAStoreInst(inst)) {
                // If instruction "I" is a store instruction, add it to the set GEN[B]
                bbGEN.insert(inst);

                // Remove instructions killed by the current instruction from the GEN set
                for (auto it = bbGEN.begin(); it != bbGEN.end();) {
                    if (*it != inst && LLVMGetOperand(inst, 1) == LLVMGetOperand(*it, 1)) {
                        it = bbGEN.erase(it);
                    } else {
                        ++it;
                    }
                }

                // Add instructions from set "S" that are killed by the current instruction to the KILL set
                for (auto storeInst : storeSet) {
                    if (storeInst != inst && LLVMGetOperand(inst, 1) == LLVMGetOperand(storeInst, 1)) {
                        bbKILL.insert(storeInst);
                    }
                }
            }
        }

        genSets[bb] = bbGEN;
        killSets[bb] = bbKILL;
    }
}

/* Helper function to print a set of LLVM values */
void printValueSet(const set<LLVMValueRef>& valueSet) {
    for (const auto& value : valueSet) {
        LLVMDumpValue(value);
        printf(" ");
    }
    printf("\n");
}

/* Computes the IN and OUT sets for each basic block using the GEN and KILL sets. */
void computeInOutSets(LLVMValueRef function, const BBValueSetMap& genSets, const BBValueSetMap& killSets,
                      BBValueSetMap& inSets, BBValueSetMap& outSets, const BBPredMap& predMap) {
    // For each basic block B, set OUT[B] = GEN[B]
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        outSets[bb] = genSets.find(bb)->second;
    }

    bool changed = true;
    while (changed) {
        changed = false;

        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
            set<LLVMValueRef> newInSet, newOutSet;

            // IN[B] = union(OUT[P1], OUT[P2], ..., OUT[PN]),
            // where P1, P2, ..., PN are predecessors of basic block B
            auto predIt = predMap.find(bb);
            if (predIt != predMap.end()) {
                for (const auto& pred : predIt->second) {
                    const auto& predOutSet = outSets.find(pred)->second;
                    newInSet.insert(predOutSet.begin(), predOutSet.end());
                }
            }

            // oldout = OUT[B]
            const auto& oldOutSet = outSets.find(bb)->second;

            // OUT[B] = GEN[B] union (IN[B] - KILL[B])
            const auto& bbGenSet = genSets.find(bb)->second;
            const auto& bbKillSet = killSets.find(bb)->second;
            set_difference(newInSet.begin(), newInSet.end(), bbKillSet.begin(), bbKillSet.end(), inserter(newOutSet, newOutSet.end()));
            newOutSet.insert(bbGenSet.begin(), bbGenSet.end());

            // if (OUT[B] != oldout) change = True
            if (newOutSet != oldOutSet) {
                changed = true;
                outSets[bb] = newOutSet;
            }

            inSets[bb] = newInSet;
        }
    }
}

/* Calculates the predecessor map for the given function. */
BBPredMap calculatePredecessorMap(LLVMValueRef function) {
    BBPredMap predMap;

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        LLVMValueRef terminator = LLVMGetBasicBlockTerminator(bb);
        if (terminator) {
            unsigned numSuccessors = LLVMGetNumSuccessors(terminator);
            for (unsigned i = 0; i < numSuccessors; ++i) {
                LLVMBasicBlockRef successor = LLVMGetSuccessor(terminator, i);
                predMap[successor].push_back(bb);
            }
        }
    }

    return predMap;
}

/* Performs constant propagation on the given function. */
bool constantPropagation(LLVMValueRef function, const BBPredMap& predMap) {
    printf("\nGlobal Optimizations\n");

    bool changed = false;

    BBValueSetMap genSets, killSets, inSets, outSets;
    computeGenKillSets(function, genSets, killSets);
    computeInOutSets(function, genSets, killSets, inSets, outSets, predMap);

    // Walk through each basic block B
    for (auto& entry : inSets) {
        LLVMBasicBlockRef bb = entry.first;
        set<LLVMValueRef> reachingStores = entry.second;  // R = IN[B]
        vector<LLVMValueRef> markedLoads;

        // For every instruction I in B
        for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
            if (LLVMIsAStoreInst(inst)) {
                // If I is a store instruction, add I to R
                reachingStores.insert(inst);

                // If I is a store instruction, remove all store instructions in R that are killed by I
                for (auto it = reachingStores.begin(); it != reachingStores.end();) {
                    if (*it != inst && LLVMGetOperand(inst, 1) == LLVMGetOperand(*it, 1)) {
                        it = reachingStores.erase(it);
                    } else {
                        ++it;
                    }
                }
            } else if (LLVMIsALoadInst(inst)) {
                // If I is a load instruction that loads from address represented by variable %t
                LLVMValueRef loadAddr = LLVMGetOperand(inst, 0);
                LLVMValueRef constValue = nullptr;
                bool allConstant = true;

                // Find all the store instructions in R that write to address represented by %t
                for (LLVMValueRef storeInst : reachingStores) {
                    if (LLVMGetOperand(storeInst, 1) == loadAddr) {
                        LLVMValueRef storeValue = LLVMGetOperand(storeInst, 0);
                        if (LLVMIsConstant(storeValue)) {
                            if (constValue == nullptr) {
                                constValue = storeValue;
                            } else if (constValue != storeValue) {
                                allConstant = false;
                                break;
                            }
                        } else {
                            allConstant = false;
                            break;
                        }
                    }
                }

                // If all these store instructions are constant store instructions and write the same constant value
                if (allConstant && constValue != nullptr) {
                    // Replace all uses of instruction I (load instruction) by the constant in store instructions
                    LLVMReplaceAllUsesWith(inst, constValue);
                    // Mark load instruction for deletion
                    markedLoads.push_back(inst);
                    changed = true;
                }
            }
        }

        // Delete all the marked load instructions
        printf("Deleting marked loads:\n");
        for (LLVMValueRef loadInst : markedLoads) {
            LLVMDumpValue(loadInst);
            printf("\n");
            LLVMInstructionEraseFromParent(loadInst);
        }
    }

    return changed;
}

void walkFunctions(LLVMModuleRef module) {
    for (LLVMValueRef function = LLVMGetFirstFunction(module);
         function;
         function = LLVMGetNextFunction(function)) {
        const char* funcName = LLVMGetValueName(function);

        printf("Function Name: %s\n", funcName);

        BBPredMap predMap = calculatePredecessorMap(function);
        bool changed;
        do {
            localOptimizations(function);
            changed = constantPropagation(function, predMap);
        } while (changed);
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

void llvm_parse(const char* llFile, const char* outFile) {
    LLVMModuleRef m = createLLVMModel(llFile);

    if (m != NULL) {
        // LLVMDumpModule(m);
        walkGlobalValues(m);
        walkFunctions(m);
        LLVMPrintModuleToFile(m, outFile, NULL);
        LLVMDisposeModule(m);
    } else {
        fprintf(stderr, "m is NULL\n");
    }

    LLVMShutdown();
}
