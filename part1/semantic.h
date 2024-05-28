#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"

using namespace std;

class SymbolTable {
   public:
    void insert(const string& identifier, int value);
    bool exists(const string& identifier);

   private:
    unordered_map<string, int> table;
};

class SemanticAnalyzer {
   public:
    bool analyze(astNode* root);

   private:
    vector<SymbolTable> symbol_tables;

    void new_scope();
    void end_scope();
    void insert(const string& identifier, int value = 0);
    bool exists(const string& identifier);
    void traverse(astNode* node);
};

void yyerror(const char*);
bool runParser(const char* filename);
bool runSemanticAnalysis(bool cleanup);

#endif  // SEMANTIC_H