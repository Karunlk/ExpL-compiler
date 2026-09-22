%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task3.h"
#include "y.tab.h"
int yywrap(void) { return 1; }
%}
%%
"decl"      { return DECL; }
"enddecl"   { return ENDDECL; }
"begin"     { return BEGIN_TOKEN; }
"end"       { return END_TOKEN; }
"read"      { return READ; }
"write"     { return WRITE; }
"return"    { return RETURN; }
"if"        { return IF; }
"then"      { return THEN; }
"else"      { return ELSE; }
"endif"     { return ENDIF; }
"while"     { return WHILE; }
"do"        { return DO; }
"endwhile"  { return ENDWHILE; }
"AND"       { return AND; }
"int"       { return INT; }
"str"       { return STR; }
"<="        { return LE; }
">="        { return GE; }
"!="        { return NE; }
"=="        { return EQ; }
"<"         { return LT; }
">"         { return GT; }
[0-9]+      { yylval.val = atoi(yytext); return NUM; }
\"[^\"]*\" { yylval.str = strdup(yytext); return STRCONST; }
[a-zA-Z][a-zA-Z0-9_]* { yylval.str = strdup(yytext); return ID; }
[ \t\n]+    ;
.           { return yytext[0]; }
%%
