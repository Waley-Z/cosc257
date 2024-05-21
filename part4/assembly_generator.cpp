#include "assembly_generator.h"

#include <algorithm>
#include <cstring>
#include <iostream>

using namespace std;

extern LLVMModuleRef createLLVMModel(const char* inputFilename);

const char* AssemblyGenerator::REGS[NUM_REGS] = {"ebx", "ecx", "edx"};

AssemblyGenerator::AssemblyGenerator(const char* _inputFilename, const char* _outputFilename)
    : outputFilename(_outputFilename) {
    module = createLLVMModel(_inputFilename);
}

void AssemblyGenerator::generateInstIndexMap(LLVMBasicBlockRef bb) {
    int count = 0;
    for (auto inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
        if (!LLVMIsAAllocaInst(inst)) {
            instIndex[inst] = count++;
        }
    }
}

void AssemblyGenerator::computeLiveness(LLVMBasicBlockRef bb) {
    int count = 0;
    for (auto inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
        if (LLVMIsAAllocaInst(inst)) continue;

        if (LLVMTypeOf(inst) != LLVMVoidType()) {
            liveRange[inst].first = count;
        }

        int endUser = 0;
        for (auto use = LLVMGetFirstUse(inst); use; use = LLVMGetNextUse(use)) {
            endUser = max(endUser, instIndex[LLVMGetUser(use)]);
        }
        liveRange[inst].second = endUser;
        count++;
    }
}

int AssemblyGenerator::countNumUses(LLVMValueRef value) {
    int count = 0;
    for (auto use = LLVMGetFirstUse(value); use; use = LLVMGetNextUse(use)) {
        count++;
    }
    return count;
}

bool AssemblyGenerator::compareUses(LLVMValueRef a, LLVMValueRef b) {
    return countNumUses(a) < countNumUses(b);
}

void AssemblyGenerator::regAllocation(LLVMBasicBlockRef bb) {
    bool available[NUM_REGS] = {true};
    vector<LLVMValueRef> allInst;

    for (auto inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
        if (LLVMIsAAllocaInst(inst) || LLVMTypeOf(inst) == LLVMVoidType()) continue;
        allInst.push_back(inst);
    }

    sort(allInst.begin(), allInst.end(), [this](LLVMValueRef a, LLVMValueRef b) { return compareUses(a, b); });

    int index = 0;
    for (auto inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
        for (int i = 0; i < LLVMGetNumOperands(inst); i++) {
            auto operand = LLVMGetOperand(inst, i);
            if (LLVMIsAInstruction(operand) && liveRange.count(operand) && regMap.count(operand)) {
                cout << "Instruction: " << LLVMPrintValueToString(inst) << endl;
                cout << "Used Instruction:" << LLVMPrintValueToString(operand) << endl;
                cout << "current index:" << index << " " << "end index:" << liveRange[operand].second << endl;
                if (liveRange[operand].second == index) {
                    int j = -1;

                    for (int i = 0; i < NUM_REGS; i++) {
                        if (strcmp(REGS[i], regMap[operand]) == 0) {
                            j = i;
                        }
                    }

                    if (j >= 0) {
                        available[j] = true;
                        cout << "Freeing " << regMap[operand] << endl;
                    }
                }
            }
        }

        if (LLVMIsAAllocaInst(inst)) continue;

        index++;

        if (LLVMTypeOf(inst) == LLVMVoidType()) continue;

        int regIndex = -1;
        for (int i = 0; i < NUM_REGS; i++) {
            if (available[i]) {
                regIndex = i;
                break;
            }
        }

        if (regIndex != -1) {
            available[regIndex] = false;
            regMap[inst] = REGS[regIndex];
        } else {
            LLVMValueRef spill = nullptr;

            for (auto i : allInst) {
                if (compareUses(i, inst)) continue;
                if (regMap.count(i) && strcmp(regMap[i], "-1")) {
                    spill = i;
                }
            }

            if (spill) {
                regMap[inst] = regMap[spill];
                regMap[spill] = "-1";
            } else {
                regMap[inst] = "-1";
            }
        }
    }
}

void AssemblyGenerator::createBBLabels(LLVMValueRef function) {
    int i = 0;
    for (auto bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        bbLabels[bb] = (i == 0) ? ".LFB" + to_string(i++) : ".L" + to_string(i++);
    }
}

void AssemblyGenerator::printDirectives(LLVMValueRef function, int offset) {
    cout << LLVMGetValueName(function) << ":\n";
    cout << bbLabels[LLVMGetFirstBasicBlock(function)] << ":" << endl;
    cout << "\tpushl\t%ebp\n";
    cout << "\tmovl\t%esp, %ebp\n";
    cout << "\tsubl\t$" << offset << ", %esp\n";
}

int AssemblyGenerator::getOffsetMap(LLVMValueRef function) {
    int localMem = 0;
    const int SIZE = 4;
    for (auto bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        for (auto inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
            if (LLVMIsAAllocaInst(inst)) {
                offsetMap[inst] = -(localMem += SIZE);
            }
        }
    }
    for (const auto& [key, value] : regMap) {
        if (!strcmp(value, "-1")) {
            offsetMap[key] = -(localMem += SIZE);
        }
    }
    return localMem - SIZE;
}

void AssemblyGenerator::codeGeneration() {
    cout << "\t.text\n";
    cout << "\t.globl\tfunc\n";
    cout << "\t.type\tfunc, @function\n";

    for (auto function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
        int offset = getOffsetMap(function);
        printDirectives(function, offset);
        generateFunctionCode(function);
    }
}

void AssemblyGenerator::generateFunctionCode(LLVMValueRef function) {
    for (auto bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        if (bb != LLVMGetFirstBasicBlock(function)) {
            cout << bbLabels[bb] << ":" << endl;
        }
        generateBasicBlockCode(bb);
    }
}

void AssemblyGenerator::generateBasicBlockCode(LLVMBasicBlockRef bb) {
    for (auto inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
        generateInstructionCode(inst);
    }
}

void AssemblyGenerator::generateInstructionCode(LLVMValueRef inst) {
    if (LLVMIsAReturnInst(inst)) {
        generateReturnCode(inst);
    } else if (LLVMIsALoadInst(inst)) {
        generateLoadCode(inst);
    } else if (LLVMIsAStoreInst(inst)) {
        generateStoreCode(inst);
    } else if (LLVMIsACallInst(inst)) {
        generateCallCode(inst);
    } else if (LLVMIsABranchInst(inst)) {
        generateBranchCode(inst);
    } else if (LLVMIsAAllocaInst(inst)) {
        return;
    } else {
        generateArithmeticCode(inst);
    }
}

void AssemblyGenerator::generateReturnCode(LLVMValueRef inst) {
    auto operand = LLVMGetOperand(inst, 0);
    if (LLVMIsAConstant(operand)) {
        cout << "\tmovl\t$" << LLVMConstIntGetSExtValue(operand) << ", %eax\n";
    } else if (offsetMap.count(operand)) {
        cout << "\tmovl\t" << offsetMap[operand] << "(%ebp), %eax\n";
    } else {
        cout << "\tmovl\t%" << regMap[operand] << ", %eax\n";
    }
    cout << "\tleave\n";
    cout << "\tret\n";
}

void AssemblyGenerator::generateLoadCode(LLVMValueRef inst) {
    auto dst = inst;
    auto src = LLVMGetOperand(inst, 0);
    if (strcmp(regMap[dst], "-1")) {
        cout << "\tmovl\t" << offsetMap[src] << "(%ebp), %" << regMap[dst] << endl;
    }
}

void AssemblyGenerator::generateStoreCode(LLVMValueRef inst) {
    auto src = LLVMGetOperand(inst, 0);
    auto dst = LLVMGetOperand(inst, 1);
    if (!LLVMIsAArgument(src)) {
        if (LLVMIsAConstant(src)) {
            cout << "\tmovl\t$" << LLVMConstIntGetZExtValue(src) << ", " << offsetMap[dst] << "(%ebp)\n";
        } else {
            if (strcmp(regMap[src], "-1")) {
                cout << "\tmovl\t%" << regMap[src] << ", " << offsetMap[dst] << "(%ebp)\n";
            } else {
                cout << "\tmovl\t" << offsetMap[src] << "(%ebp), %eax\n";
                cout << "\tmovl\t%eax, " << offsetMap[dst] << "(%ebp)\n";
            }
        }
    }
}

void AssemblyGenerator::generateCallCode(LLVMValueRef inst) {
    cout << "\tpushl\t%ebx\n\tpushl\t%ecx\n\tpushl\t%edx\n";

    auto func = LLVMGetCalledValue(inst);
    int numArgs = LLVMCountParams(func);
    for (int i = numArgs - 1; i >= 0; i--) {
        auto P = LLVMGetParam(func, i);
        if (LLVMIsAConstant(P)) {
            cout << "\tpushl\t$" << LLVMConstIntGetZExtValue(P) << endl;
        } else if (strcmp(regMap[P], "-1")) {
            cout << "\tpushl\t%" << regMap[P] << endl;
        } else {
            cout << "\tpushl\t" << offsetMap[P] << "(%ebp)\n";
        }
    }

    cout << "\tcall\t" << LLVMGetValueName(func) << endl;

    if (numArgs > 0) {
        cout << "\taddl\t$" << numArgs * 4 << ", %esp\n";
    }

    cout << "\tpopl\t%edx\n\tpopl\t%ecx\n\tpopl\t%ebx\n";

    if (LLVMGetInstructionCallConv(inst) == LLVMCCallConv) {
        if (strcmp(regMap[inst], "-1")) {
            cout << "\tmovl\t%eax, %" << regMap[inst] << endl;
        } else {
            cout << "\tmovl\t%eax, " << offsetMap[inst] << "(%ebp)\n";
        }
    }
}

void AssemblyGenerator::generateBranchCode(LLVMValueRef inst) {
    unsigned numOperands = LLVMGetNumOperands(inst);
    if (numOperands == 1) {
        auto bb = LLVMValueAsBasicBlock(LLVMGetOperand(inst, 0));
        cout << "\tjmp " << bbLabels[bb] << endl;
    } else if (numOperands == 3) {
        auto bb1 = LLVMValueAsBasicBlock(LLVMGetOperand(inst, 1));
        auto bb2 = LLVMValueAsBasicBlock(LLVMGetOperand(inst, 2));
        auto cond = LLVMGetOperand(inst, 0);
        auto T = LLVMGetICmpPredicate(cond);
        switch (T) {
            case LLVMIntEQ:
                cout << "\tje " << bbLabels[bb1] << endl;
                break;
            case LLVMIntNE:
                cout << "\tjne " << bbLabels[bb1] << endl;
                break;
            case LLVMIntSGT:
                cout << "\tjg " << bbLabels[bb1] << endl;
                break;
            case LLVMIntSGE:
                cout << "\tjge " << bbLabels[bb1] << endl;
                break;
            case LLVMIntSLT:
                cout << "\tjl " << bbLabels[bb1] << endl;
                break;
            case LLVMIntSLE:
                cout << "\tjle " << bbLabels[bb1] << endl;
                break;
            default:
                break;
        }
        cout << "\tjmp " << bbLabels[bb2] << endl;
    }
}

void AssemblyGenerator::generateArithmeticCode(LLVMValueRef inst) {
    auto opcode = LLVMGetInstructionOpcode(inst);
    if (opcode == LLVMAdd || opcode == LLVMICmp || opcode == LLVMSub || opcode == LLVMMul) {
        string X = (strcmp(regMap[inst], "-1")) ? "%" + string(regMap[inst]) : "%eax";
        auto A = LLVMGetOperand(inst, 0);
        auto B = LLVMGetOperand(inst, 1);
        if (LLVMIsConstant(A)) {
            cout << "\tmovl\t$" << LLVMConstIntGetSExtValue(A) << ", " << X << endl;
        } else if (strcmp(regMap[A], "-1")) {
            cout << "\tmovl\t%" << regMap[A] << ", " << X << endl;
        } else if (offsetMap.count(A)) {
            cout << "\tmovl\t" << offsetMap[A] << "(%ebp), " << X << endl;
        }
        string op;
        switch (opcode) {
            case LLVMAdd:
                op = "\taddl\t";
                break;
            case LLVMICmp:
                op = "\tcmpl\t";
                break;
            case LLVMSub:
                op = "\tsubl\t";
                break;
            case LLVMMul:
                op = "\timull\t";
                break;
            default:
                break;
        }
        if (LLVMIsConstant(B)) {
            cout << op << "$" << LLVMConstIntGetSExtValue(B) << ", " << X << endl;
        } else if (strcmp(regMap[B], "-1")) {
            cout << op << "%" << regMap[B] << ", " << X << endl;
        } else if (offsetMap.count(B)) {
            cout << op << offsetMap[B] << "(%ebp), " << X << endl;
        }
        if (offsetMap.count(inst)) {
            cout << "\tmovl\t%eax, " << offsetMap[inst] << "(%ebp)\n";
        }
    }
}

void AssemblyGenerator::walkBasicBlocks(LLVMValueRef function) {
    for (auto bb = LLVMGetFirstBasicBlock(function); bb; bb = LLVMGetNextBasicBlock(bb)) {
        generateInstIndexMap(bb);
        computeLiveness(bb);
        regAllocation(bb);
        for (const auto& [inst, range] : liveRange) {
            cout << "Instruction: " << LLVMPrintValueToString(inst)
                 << ", Start: " << range.first << ", End: " << range.second << endl;
        }
        cout << endl;
        instIndex.clear();
        liveRange.clear();
    }
}

void AssemblyGenerator::walkFunctionsAssembly() {
    for (auto function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
        printf("\nFunction Name: %s\n", LLVMGetValueName(function));
        walkBasicBlocks(function);
        getOffsetMap(function);
        createBBLabels(function);
    }

    for (const auto& [inst, reg] : regMap) {
        printf("Instruction: %s -> Register: %s\n", LLVMPrintValueToString(inst), reg);
    }
    cout << endl;
    for (const auto& [value, offset] : offsetMap) {
        cout << LLVMPrintValueToString(value) << " -> " << offset << endl;
    }
}

void AssemblyGenerator::generateAssembly() {
    if (module) {
        walkFunctionsAssembly();
        freopen(outputFilename, "w", stdout);
        codeGeneration();
        LLVMDisposeModule(module);
    }
    LLVMShutdown();
}
