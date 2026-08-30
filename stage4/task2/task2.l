%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "y.tab.h"
    int yywrap(void)
    {
        return 1;
    }
%}

%%

"begin"                 { return BEGIN_TOKEN; }
"end"                   { return END_TOKEN; }
"read"                  { return READ; }
"write"                 { return WRITE; }

"if"                    { return IF; }
"then"                  { return THEN; }
"else"                  { return ELSE; }
"endif"                 { return ENDIF; }

"while"                 { return WHILE; }
"do"                    { return DO; }
"endwhile"              { return ENDWHILE; }

"repeat"                { return REPEAT; }
"until"                 { return UNTIL; }

"break"                 { return BREAK; }
"continue"              { return CONTINUE; }

"decl"                  { return DECL; }
"int"                   { return INT; }
"str"                   { return STR; }
"enddecl"               { return ENDDECL; }

"<="                    { return LE; }
">="                    { return GE; }
"!="                    { return NE; }
"=="                    { return EQ; }
"<"                     { return LT; }
">"                     { return GT; }

[0-9]+                  {yylval.val = atoi(yytext);return NUM;}

\"[^\"]*\"              {yylval.str = strdup(yytext); return STRCONST;}

[a-zA-Z][a-zA-Z0-9_]*   {yylval.str = strdup(yytext); return ID;}

[ \t\n]+                ;

"["                     { return '['; }
"]"                     { return ']'; }
"("                     { return '('; }
")"                     { return ')'; }
";"                     { return ';'; }
","                     { return ','; }
"="                     { return '='; }
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }

.                       { }

%%