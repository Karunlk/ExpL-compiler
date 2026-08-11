%{
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include "y.tab.h"
    #include "task1.h"
%}

%%
"begin"             { return BEGIN_TOKEN; }
"end"               { return END_TOKEN; }
"read"              { return READ; }
"write"             { return WRITE; }
[0-9]+              {
                        yylval.val = atoi(yytext);
                        return NUM;
                    }
[a-z]               {
                        yylval.str = strdup(yytext);
                        return ID;
                    }
[ \t\n]+            {}
"("                 { return '('; }
")"                 { return ')'; }
";"                 { return ';'; }
"="                 { return '='; }
"+"                 { return '+'; }
"-"                 { return '-'; }
"*"                 { return '*'; }
"/"                 { return '/'; }
.                   {
                        printf("Unknown character: %s\n", yytext);
                    }
%%