%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "task1.h"
#include "y.tab.h"

int yywrap(void)
{
    return 1;
}
%}

%%

"decl"                  { return DECL; }
"enddecl"               { return ENDDECL; }
"int"                   { return INT; }
"str"                   { return STR; }

[0-9]+                  { yylval.val = atoi(yytext); return NUM; }
[a-zA-Z][a-zA-Z0-9_]*  { yylval.str = strdup(yytext); return ID; }
\"[^\"]*\"              { yylval.str = strdup(yytext); return STRCONST; }

[ \t\n]+                ;

"["                     { return '['; }
"]"                     { return ']'; }
"("                     { return '('; }
")"                     { return ')'; }
";"                     { return ';'; }
","                     { return ','; }
"*"                     { return '*'; }
"{"                     { return '{'; }
"}"                     { return '}'; }

.                       { return yytext[0]; }

%%
