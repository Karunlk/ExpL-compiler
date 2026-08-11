%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "y.tab.h"
    #include "task2.h"
%}


%%
"begin"         {return BEGIN_TOKEN;}
"end"           {return END_TOKEN;}
"read"          {return READ;}
"write"         {return WRITE;}
[0-9]+          {yylval.val = atoi(yytext);return NUM;}
[a-z]           {yylval.str = strdup(yytext);return ID;}
[ \t\n\r]+      {}
"("             { return '('; }
")"             { return ')'; }
";"             { return ';'; }
"="             { return '='; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
.               {printf("Invalid character: %s\n",yytext);}
%%