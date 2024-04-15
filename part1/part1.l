%{
	#include <stdio.h>
	#include <string.h>
	#include "ast.h"
	#include "y.tab.h"
%}

%option yylineno

digit       [0-9]
letter      [a-zA-Z]
alphanum_us [a-zA-Z0-9_]

%%
"int"       { return INT; }
"void"      { return VOID; }
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"return"    { return RETURN; }
"extern"    { return EXTERN; }

"<"         { return LT; }
">"         { return GT; }
"<="        { return LE; }
">="        { return GE; }
"=="        { return EQ; }

{letter}{alphanum_us}*  { yylval.idname = strdup(yytext); return IDENTIFIER; }
{digit}+                { yylval.ival = atoi(yytext); return NUMBER; }

[ \t\n]
.           { return yytext[0]; }
%%

int yywrap(){
	return 1;
}
