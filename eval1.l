%option noyywrap

%{

#include<stdio.h>
#include<stdlib.h>

%}

number  [0-9]+
op      [+\-\*/]

%%
{number}    {printf("Number\tLength: %d\n",yyleng);}
{op}        {printf("Operator\tLength: %d\n",yyleng);}
%%

int main()
{
    yylex();
    return 0;
}