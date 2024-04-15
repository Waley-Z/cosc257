%{
#include <cstdio>
#include "ast.h"
#include "part1_semantic.cpp"
extern int yylex();
extern int yylex_destroy();
void yyerror(const char *);
extern FILE *yyin;
extern int yylineno;

astNode* root;
%}

%union {
    int ival;
    char *idname;
    astNode *nptr;
    vector<astNode*> *svec_ptr;
}

%token <idname> IDENTIFIER PRINT READ
%token <ival> NUMBER
%token INT VOID IF ELSE WHILE RETURN EXTERN
%token LT GT LE GE EQ

%type <svec_ptr> statements var_declarations
%type <nptr> program extern_declaration function block
%type <nptr> var_declaration statement
%type <nptr> condition expression arithmetic_expression call_expression terminal

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%left '+' '-'
%left '*' '/'
%right UMINUS

%start program
%%

program:
    extern_declaration extern_declaration function {
        $$ = createProg($1, $2, $3);
        root = $$;
    }

extern_declaration:
    EXTERN VOID IDENTIFIER '(' INT ')' ';' {
        $$ = createExtern($3);
        free($3);
    }
  | EXTERN INT IDENTIFIER '(' ')' ';' {
        $$ = createExtern($3);
        free($3);
    }

function:
    INT IDENTIFIER '(' INT IDENTIFIER ')' block {
        $$ = createFunc($2, createVar($5), $7);
        free($2);
        free($5);
    }
  | INT IDENTIFIER '(' ')' block {
        $$ = createFunc($2, nullptr, $5);
        free($2);
    }

block:
    '{' var_declarations statements '}' {
        $2->insert($2->end(), $3->begin(), $3->end()); 
        $$ = createBlock($2);
        delete($3);
    }
  | '{' statements '}' {
        $$ = createBlock($2);
    }
  | '{' var_declarations '}' {
        $$ = createBlock($2);
    }
  | '{' '}' {
        $$ = createBlock(new vector<astNode*>());
    }

var_declarations:
    var_declarations var_declaration {
        $$ = $1;
        $$->push_back($2);
    }
  | var_declaration {
        $$ = new vector<astNode*>();
        $$->push_back($1);
    }

var_declaration:
    INT IDENTIFIER ';' {
        $$ = createDecl($2);
        free($2);
    }

statements:
    statements statement {
        $$ = $1;
        $$->push_back($2);
    }
  | statement {
        $$ = new vector<astNode*>();
        $$->push_back($1);
    }

statement:
    expression ';' { $$ = $1; }
  | block { $$ = $1; }
  | IDENTIFIER '=' expression ';' {
        $$ = createAsgn(createVar($1), $3);
        free($1);
    }
  | IF '(' condition ')' statement ELSE statement {
        $$ = createIf($3, $5, $7);
    }
  | IF '(' condition ')' statement %prec LOWER_THAN_ELSE {
        $$ = createIf($3, $5, nullptr);
    }
  | WHILE '(' condition ')' statement {
        $$ = createWhile($3, $5);
    }
  | RETURN expression ';' {
        $$ = createRet($2);
    }

condition:
    expression GT expression { $$ = createRExpr($1, $3, gt); }
  | expression LT expression { $$ = createRExpr($1, $3, lt); }
  | expression EQ expression { $$ = createRExpr($1, $3, eq); }
  | expression GE expression { $$ = createRExpr($1, $3, ge); }
  | expression LE expression { $$ = createRExpr($1, $3, le); }

expression:
    terminal { $$ = $1; }
  | '(' expression ')' { $$ = $2; }
  | call_expression { $$ = $1; }
  | arithmetic_expression { $$ = $1; }

call_expression:
    IDENTIFIER '(' ')' { $$ = createCall($1, nullptr); free($1); }
  | IDENTIFIER '(' expression ')' { $$ = createCall($1, $3); free($1); }

arithmetic_expression:
    expression '+' expression { $$ = createBExpr($1, $3, add); }
  | expression '-' expression { $$ = createBExpr($1, $3, sub); }
  | expression '*' expression { $$ = createBExpr($1, $3, mul); }
  | expression '/' expression { $$ = createBExpr($1, $3, divide); }
  | '-' expression %prec UMINUS { $$ = createUExpr($2, uminus); }

terminal:
    NUMBER { $$ = createCnst($1); }
  | IDENTIFIER { $$ = createVar($1); free($1); }

%%

int main(int argc, char** argv){
	if (argc == 2)
  	    yyin = fopen(argv[1], "r");

	yyparse();
	
    if (root != nullptr) {
        printNode(root);
        SemanticAnalyzer sa;
        if (!sa.analyze(root)) {
            return 1;
        }
        freeNode(root);
    }

	if (yyin != stdin)
		fclose(yyin);
	yylex_destroy();

	return 0;
}

void yyerror(const char *) {
    fprintf(stderr, "Syntax Error: line %d\n", yylineno);
}
