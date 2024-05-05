#include "semantic.h"

#include <iostream>
#include <stdexcept>

void SymbolTable::insert(const string& identifier, int value) {
    if (exists(identifier)) {
        throw runtime_error("Variable '" + identifier + "' already declared in this scope.");
    }
    table[identifier] = value;
}

bool SymbolTable::exists(const string& identifier) {
    return table.find(identifier) != table.end();
}

bool SemanticAnalyzer::analyze(astNode* root) {
    try {
        traverse(root);
        return true;
    } catch (const runtime_error& e) {
        cerr << "Semantic error: " << e.what() << endl;
        return false;
    }
}

void SemanticAnalyzer::new_scope() {
    symbol_tables.push_back(SymbolTable());
}

void SemanticAnalyzer::end_scope() {
    symbol_tables.pop_back();
}

void SemanticAnalyzer::insert(const string& identifier, int value) {
    symbol_tables.back().insert(identifier, value);
}

bool SemanticAnalyzer::exists(const string& identifier) {
    for (auto it = symbol_tables.rbegin(); it != symbol_tables.rend(); ++it) {
        if (it->exists(identifier)) {
            return true;
        }
    }
    return false;
}

void SemanticAnalyzer::traverse(astNode* node) {
    if (!node) return;

    switch (node->type) {
        case ast_prog:
            traverse(node->prog.func);
            break;

        case ast_func:
            new_scope();
            if (node->func.param) {
                insert(node->func.param->var.name);
            }
            for (auto& stmt : *node->func.body->stmt.block.stmt_list) {
                traverse(stmt);
            }
            end_scope();
            break;

        case ast_stmt:
            switch (node->stmt.type) {
                case ast_call:
                    traverse(node->stmt.call.param);
                    break;

                case ast_ret:
                    traverse(node->stmt.ret.expr);
                    break;

                case ast_block:
                    new_scope();
                    for (auto& stmt : *node->stmt.block.stmt_list) {
                        traverse(stmt);
                    }
                    end_scope();
                    break;

                case ast_while:
                    traverse(node->stmt.whilen.cond);
                    traverse(node->stmt.whilen.body);
                    break;

                case ast_if:
                    traverse(node->stmt.ifn.cond);
                    traverse(node->stmt.ifn.if_body);
                    traverse(node->stmt.ifn.else_body);
                    break;

                case ast_asgn:
                    traverse(node->stmt.asgn.lhs);
                    traverse(node->stmt.asgn.rhs);
                    break;

                case ast_decl:
                    insert(node->stmt.decl.name);
                    break;

                default:
                    break;
            }
            break;

        case ast_extern:
            break;

        case ast_var:
            if (!exists(node->var.name)) {
                throw runtime_error("Variable '" + string(node->var.name) + "' not declared.");
            }
            break;

        case ast_cnst:
            break;

        case ast_rexpr:
            traverse(node->rexpr.lhs);
            traverse(node->rexpr.rhs);
            break;

        case ast_bexpr:
            traverse(node->bexpr.lhs);
            traverse(node->bexpr.rhs);
            break;

        case ast_uexpr:
            traverse(node->uexpr.expr);
            break;

        default:
            break;
    }
};

extern int yyparse();
extern int yylex_destroy();
extern FILE* yyin;
extern int yylineno;
extern astNode* root;

void yyerror(const char*) {
    fprintf(stderr, "Syntax Error: line %d\n", yylineno);
}

void runParser(const char* filename) {
    yyin = fopen(filename, "r");
    yyparse();
    fclose(yyin);
    yylex_destroy();
}

bool runSemanticAnalysis() {
    if (root != nullptr) {
        printNode(root);
        SemanticAnalyzer sa;
        if (!sa.analyze(root)) {
            freeNode(root);
            return false;
        }
        freeNode(root);
    }
    return true;
}