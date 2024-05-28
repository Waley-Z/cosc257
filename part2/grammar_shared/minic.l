%{
	#include <stdio.h>
	#include <string.h>
	#include "ast.h"
	#include "y.tab.h"
%}
digit [0-9]
%%
int										{ return INT;}
void 									{ return VOID;}
print									{ return PRINT;}
read									{ return READ;}
while									{ return WHILE;}
if										{ return IF;}
else									{ return ELSE;}
extern								{ return EXTERN;}
return 								{ return RETURN;}

[a-zA-Z][a-zA-Z0-9_]*	{ yylval.idname = strdup(yytext);
												return ID;}
[0-9]{digit}*					{ yylval.ival = atoi(yytext);
												return NUM;}
">"									  { return GT;}
"<"									  { return LT;}
"=="									{ return EQ;}
">="									{ return GE;}
"<="								  { return LE;}
"!="									{ return NEQ;}
[ \t\n]
.										{return yytext[0];}
%%

int yywrap(){
	return 1;
}

