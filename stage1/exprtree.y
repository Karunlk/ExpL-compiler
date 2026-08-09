%{
    #include<stdio.h>
    #include<stdlib.h>
    #include "exprtree.h"
    #include "exprtree.c"

%}

%union{
  struct tnode *no;
}

%type <no> expr program
%type <no> NUM
%token NUM END PLUS MINUS DIV MUL
%left PLUS MINUS
%left MUL DIV

%%
program: expr END {$$=$1; printf("Answer: %d\n",evaluate($1));printf("Prefix: "); prefix($1);printf("\n"); printf("Postfix: "); postfix($1); printf("\n"); exit(1);}
       ;
expr: expr PLUS expr  {$$ = makeOperatorNode('+',$1,$3);}
    | expr MINUS expr {$$ = makeOperatorNode('-',$1,$3);}
    | expr MUL expr   {$$ = makeOperatorNode('*',$1,$3);}
    | expr DIV expr   {$$ = makeOperatorNode('/',$1,$3);}
    | '('expr ')'     {$$ = $2;}
    |   NUM           {$$ = $1;}
    ;

%%

yyerror(char const *s)
{
    printf("yyerror: %s",s);
}

int main(void)
{
    yyparse();
    return 0;
}