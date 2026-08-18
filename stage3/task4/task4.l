%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "y.tab.h"
%}

%%

"begin"             { return BEGIN_TOKEN; }
"end"               { return END_TOKEN; }
"read"              { return READ; }
"write"             { return WRITE; }

"if"                { return IF; }
"then"              { return THEN; }
"else"              { return ELSE; }
"endif"             { return ENDIF; }

"while"             { return WHILE; }
"do"                { return DO; }
"endwhile"          { return ENDWHILE; }
"break"             { return BREAK; }
"continue"          { return CONTINUE; }

"<="                { return LE; }
">="                { return GE; }
"!="                { return NE; }
"=="                { return EQ; }
"<"                 { return LT; }
">"                 { return GT; }

[0-9]+              {yylval.val = atoi(yytext);return NUM;}

[a-z]+              {yylval.str = strdup(yytext); return ID;}

[ \t\n]+            ;

"("                 { return '('; }
")"                 { return ')'; }
";"                 { return ';'; }
"="                 { return '='; }
"+"                 { return '+'; }
"-"                 { return '-'; }
"*"                 { return '*'; }
"/"                 { return '/'; }

.                   { }

%%