#include "ir_builder.h"

#include <iostream>
#include <queue>
#include <unordered_set>

extern astNode* root;

LLVMModuleRef IRBuilder::buildIR() {
    module = LLVMModuleCreateWithName("miniC");
    LLVMSetTarget(module, "x86_64-pc-linux-gnu");

    LLVMTypeRef printParamTypes[] = {LLVMInt32Type()};
    LLVMTypeRef printFuncType = LLVMFunctionType(LLVMVoidType(), printParamTypes, 1, 0);
    LLVMAddFunction(module, "print", printFuncType);

    LLVMTypeRef readFuncType = LLVMFunctionType(LLVMInt32Type(), nullptr, 0, 0);
    LLVMAddFunction(module, "read", readFuncType);

    // Traverse the AST and build the LLVM IR
    if (root && root->type == ast_prog) {
        buildFunction(root->prog.func);
    }

    freeNode(root);

    return module;
}

void IRBuilder::buildFunction(astNode* func_node) {
    if (!func_node || func_node->type != ast_func) return;

    LLVMTypeRef paramTypes[] = {LLVMInt32Type()};
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), paramTypes, 1, 0);
    LLVMValueRef func = LLVMAddFunction(module, func_node->func.name, funcType);

    LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(func, "entry");

    builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entryBB);

    unordered_map<string, LLVMValueRef> var_map;

    LLVMValueRef param = LLVMGetParam(func, 0);
    LLVMValueRef paramAlloca = LLVMBuildAlloca(builder, LLVMInt32Type(), "p");
    LLVMBuildStore(builder, param, paramAlloca);
    var_map[func_node->func.param->var.name] = paramAlloca;

    for (astNode* stmt : *func_node->func.body->stmt.block.stmt_list) {
        if (stmt->type == ast_stmt && stmt->stmt.type == ast_decl) {
            LLVMValueRef alloca = LLVMBuildAlloca(builder, LLVMInt32Type(), stmt->stmt.decl.name);
            var_map[stmt->stmt.decl.name] = alloca;
        }
    }

    LLVMValueRef retAlloca = LLVMBuildAlloca(builder, LLVMInt32Type(), "ret");

    LLVMBasicBlockRef exitBB = buildStatement(func_node->func.body, var_map, retAlloca);

    LLVMPositionBuilderAtEnd(builder, exitBB);
    LLVMValueRef retVal = LLVMBuildLoad2(builder, LLVMInt32Type(), retAlloca, "ret_val");
    LLVMBuildRet(builder, retVal);

    removeUnusedBasicBlocks(func);
    LLVMDisposeBuilder(builder);
}

LLVMBasicBlockRef IRBuilder::buildStatement(astNode* stmt_node, unordered_map<string, LLVMValueRef>& var_map, LLVMValueRef retAlloca) {
    if (!stmt_node || stmt_node->type != ast_stmt) return nullptr;

    LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(builder);

    switch (stmt_node->stmt.type) {
        case ast_asgn: {
            LLVMValueRef rhs = buildExpression(stmt_node->stmt.asgn.rhs, var_map);
            LLVMValueRef lhs = var_map[stmt_node->stmt.asgn.lhs->var.name];
            LLVMBuildStore(builder, rhs, lhs);
            return currentBB;
        }
        case ast_ret: {
            LLVMValueRef retVal = buildExpression(stmt_node->stmt.ret.expr, var_map);
            LLVMBuildStore(builder, retVal, retAlloca);
            LLVMBasicBlockRef retBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(currentBB), "end");
            LLVMBuildBr(builder, retBB);
            return retBB;
        }
        case ast_block: {
            LLVMBasicBlockRef prevBB = currentBB;
            for (astNode* stmt : *stmt_node->stmt.block.stmt_list) {
                prevBB = buildStatement(stmt, var_map, retAlloca);
            }
            return prevBB;
        }
        case ast_while: {
            LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(currentBB), "while_cond");
            LLVMBuildBr(builder, condBB);
            LLVMPositionBuilderAtEnd(builder, condBB);
            LLVMValueRef cond = buildExpression(stmt_node->stmt.whilen.cond, var_map);
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(condBB), "while_true");
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(condBB), "while_false");
            LLVMBuildCondBr(builder, cond, trueBB, falseBB);
            LLVMPositionBuilderAtEnd(builder, trueBB);

            buildStatement(stmt_node->stmt.whilen.body, var_map, retAlloca);

            LLVMBuildBr(builder, condBB);
            LLVMPositionBuilderAtEnd(builder, falseBB);
            return falseBB;
        }
        case ast_if: {
            LLVMValueRef cond = buildExpression(stmt_node->stmt.ifn.cond, var_map);
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(currentBB), "if_true");
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(currentBB), "if_false");
            LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(currentBB), "if_end");
            LLVMBuildCondBr(builder, cond, trueBB, falseBB);

            LLVMPositionBuilderAtEnd(builder, trueBB);
            buildStatement(stmt_node->stmt.ifn.if_body, var_map, retAlloca);
            LLVMBuildBr(builder, endBB);

            LLVMPositionBuilderAtEnd(builder, falseBB);
            if (stmt_node->stmt.ifn.else_body) {
                buildStatement(stmt_node->stmt.ifn.else_body, var_map, retAlloca);
            }
            LLVMBuildBr(builder, endBB);

            LLVMPositionBuilderAtEnd(builder, endBB);
            return endBB;
        }
        default:
            return nullptr;
    }
}

LLVMValueRef IRBuilder::buildExpression(astNode* expr_node, unordered_map<string, LLVMValueRef>& var_map) {
    if (!expr_node) return nullptr;

    switch (expr_node->type) {
        case ast_cnst:
            return LLVMConstInt(LLVMInt32Type(), expr_node->cnst.value, 0);
        case ast_var: {
            LLVMValueRef varAlloca = var_map[expr_node->var.name];
            return LLVMBuildLoad2(builder, LLVMInt32Type(), varAlloca, "");
        }
        case ast_uexpr: {
            LLVMValueRef operand = buildExpression(expr_node->uexpr.expr, var_map);
            return LLVMBuildSub(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), operand, "");
        }
        case ast_bexpr: {
            LLVMValueRef lhs = buildExpression(expr_node->bexpr.lhs, var_map);
            LLVMValueRef rhs = buildExpression(expr_node->bexpr.rhs, var_map);
            switch (expr_node->bexpr.op) {
                case add:
                    return LLVMBuildAdd(builder, lhs, rhs, "");
                case sub:
                    return LLVMBuildSub(builder, lhs, rhs, "");
                case mul:
                    return LLVMBuildMul(builder, lhs, rhs, "");
                case divide:
                    return LLVMBuildSDiv(builder, lhs, rhs, "");
                default:
                    return nullptr;
            }
        }
        case ast_rexpr: {
            LLVMValueRef lhs = buildExpression(expr_node->rexpr.lhs, var_map);
            LLVMValueRef rhs = buildExpression(expr_node->rexpr.rhs, var_map);
            switch (expr_node->rexpr.op) {
                case lt:
                    return LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "");
                case gt:
                    return LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "");
                case le:
                    return LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "");
                case ge:
                    return LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "");
                case eq:
                    return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "");
                case neq:
                    return LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "");
                default:
                    return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

void IRBuilder::removeUnusedBasicBlocks(LLVMValueRef func) {
    unordered_set<LLVMBasicBlockRef> visited;
    queue<LLVMBasicBlockRef> queue;

    LLVMBasicBlockRef entryBB = LLVMGetEntryBasicBlock(func);
    queue.push(entryBB);
    visited.insert(entryBB);

    while (!queue.empty()) {
        LLVMBasicBlockRef current = queue.front();
        queue.pop();

        LLVMValueRef terminator = LLVMGetBasicBlockTerminator(current);
        if (!terminator) continue;

        unsigned numSuccessors = LLVMGetNumSuccessors(terminator);
        for (unsigned i = 0; i < numSuccessors; ++i) {
            LLVMBasicBlockRef successor = LLVMGetSuccessor(terminator, i);
            if (visited.find(successor) == visited.end()) {
                queue.push(successor);
                visited.insert(successor);
            }
        }
    }

    LLVMBasicBlockRef current = LLVMGetFirstBasicBlock(func);
    while (current) {
        LLVMBasicBlockRef next = LLVMGetNextBasicBlock(current);
        if (visited.find(current) == visited.end()) {
            LLVMRemoveBasicBlockFromParent(current);
            LLVMDeleteBasicBlock(current);
        }
        current = next;
    }
}

void printLLVMIR(LLVMModuleRef module) {
    char* ir = LLVMPrintModuleToString(module);
    cout << ir << endl;
    LLVMDisposeMessage(ir);
}

bool runIRBuilder(const string &filename) {
    if (root == nullptr) {
        cerr << "AST root is nullptr. Skipping IR builder." << endl;
        return false;
    }

    IRBuilder builder;
    LLVMModuleRef m = builder.buildIR();
    printLLVMIR(m);
    LLVMPrintModuleToFile(m, filename.c_str(), nullptr);
    LLVMDisposeModule(m);

    return true;
}
