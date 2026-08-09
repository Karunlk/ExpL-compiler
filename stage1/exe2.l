%{ 
    #include<stdio.h>
    #include<stdlib.h>
    #include "exe2.h"
    #include "y.tab.h"

%}

DIGIT   [0-9]
NUM     {DIGIT}+

%%

{NUM}      {yylval.integer = atoi(yytext); return NUM;}
[ \t]      {}
"+"        {return '+';}
"-"        {return '-';}
"*"        {return '*';}
"/"        {return '/';}
"("        {return '(';}
")"        {return ')';}
"\n"       {return '\n';}
.          {fprintf(stderr,"Invalid character: %s\n",yytext); exit(1);}

%%

int yywrap(void)
    {
        return 1;
    }