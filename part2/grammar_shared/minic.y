%{
#include <stdio.h>
#include "ast.h"

extern int yylex();
extern int yylex_destroy();
extern int yywrap();
void yyerror(const char *);
extern FILE *yyin;
astNode *root;
%}

%union{
		int ival;
		char * idname;
	  astNode *nptr;
		vector<astNode *> *svec_ptr;
	}

%token <ival> NUM
%token <idname> ID
%token INT VOID PRINT READ WHILE IF ELSE EXTERN RETURN EQ GT LT GE LE NEQ

%type <svec_ptr> stmts var_decls
%type <nptr> term expr cond stmt decl block_stmt func_def extern_read extern_print prog

%nonassoc NOELSE
%nonassoc ELSE

%start prog
%%

prog	:	extern_print extern_read func_def { $$ = createProg($1, $2, $3); 
																				    root = $$;
																					}

extern_read: EXTERN INT READ '(' ')' ';' { $$ = createExtern("read");} 

extern_print: EXTERN VOID PRINT '(' INT')' ';' { $$ = createExtern("print");}

func_def   : INT ID '(' ')' block_stmt {  $$ = createFunc($2, NULL, $5); 
																					free($2);}
					 | INT ID '(' INT ID ')' block_stmt { astNode* vnptr = createVar($5);
																								$$ = createFunc($2, vnptr, $7); free($2); free($5);} 

block_stmt : '{' var_decls stmts '}' { vector<astNode*>* new_vec = new vector<astNode*>();
																			 new_vec->insert(new_vec->end(), $2->begin(), $2->end());
																			 new_vec->insert(new_vec->end(), $3->begin(), $3->end());
																			 $$ = createBlock(new_vec); 
																		   delete($2);
																			 delete($3);
																		 } 
					 | '{' stmts '}' { $$ = createBlock($2);}

var_decls	 : var_decls decl { $$ = $1;
														  $$->push_back($2);}
					 | decl { $$ = new vector<astNode*>();
									  $$->push_back($1);}

decl 			 : INT ID ';' { $$ = createDecl($2); free($2);}

stmts			 : stmts stmt { $$ = $1;
												  $$->push_back($2);}
					 | stmt { $$ = new vector<astNode*>();
									  $$->push_back($1);}

stmt			 : ID '=' expr ';' { astNode* vnptr = createVar($1);
															 $$ = createAsgn(vnptr, $3);
															 free($1); 
														 }
					 | block_stmt // Default action $$ = $1;
					 | WHILE '(' cond ')' stmt { $$ = createWhile($3, $5);} 
					 | IF '(' cond ')' stmt ELSE stmt { $$ = createIf($3, $5, $7);}
					 | IF '(' cond ')' stmt %prec NOELSE { $$ = createIf($3, $5);}
					 | PRINT '(' term ')' ';' { $$ = createCall("print", $3);}
					 | RETURN '(' expr ')' ';' { $$ = createRet($3);}
					 | RETURN expr ';' { $$ = createRet($2);}
	 

cond			 : term GT term   { $$ = createRExpr($1, $3, gt);}
					 | term LT term   { $$ = createRExpr($1, $3, lt);}
					 | term EQ term   { $$ = createRExpr($1, $3, eq);}
					 | term GE term   { $$ = createRExpr($1, $3, ge);}
					 | term LE term   { $$ = createRExpr($1, $3, le);}
					 | term NEQ term  { $$ = createRExpr($1, $3, neq);}


expr			 : term '+' term { $$ = createBExpr($1, $3, add);}
					 | term '-' term { $$ = createBExpr($1, $3, sub);}
					 | term '*' term { $$ = createBExpr($1, $3, mul);}
					 | term '/' term { $$ = createBExpr($1, $3, divide);}
					 | term { $$ = $1;}
					 | READ '(' ')' { $$ = createCall("read");}			 

term			 : NUM { $$ = createCnst($1);}
					 | ID { $$ = createVar($1); free($1);}
					 | '-' term { $$ = createUExpr($2, uminus);}


%%
int main(int argc, char** argv){
	if (argc == 2){
  	yyin = fopen(argv[1], "r");
		if (yyin == NULL) {
			return 1;
		}
	}
	
	#ifdef YYDEBUG
  	yydebug = 0;
	#endif 

	if (yyparse() != 0) {
		if (yyin != stdin)
			fclose(yyin);
	
		yylex_destroy();
	
		return 1;
	}
	
	if (root != NULL) {
			printNode(root);
	}

	if (yyin != stdin)
		fclose(yyin);
	
	yylex_destroy();
	
	return 0;
}

void yyerror(const char *){
	fprintf(stderr, "Syntax error\n");
}
