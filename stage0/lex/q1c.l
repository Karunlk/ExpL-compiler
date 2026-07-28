%option noyywrap

%{

    #include<stdio.h>
%}



%%
A[A-Za-z][A-Za-z]  { printf("%s\n", yytext); }
[ \t\r\n]+        { /* skip whitespace */ }
.                 { /* skip everything else */ }
%%

int main()
{
    printf("Input the words (Ctrl+D to end):\n");
    
    yylex();
    return 1;
}