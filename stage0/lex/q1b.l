%option noyywrap

%{
    #include<stdio.h>
    #include<stdlib.h>

    int int_count = 0;
    int float_count = 0;
%}


digit       [0-9]
integer     [+-]?{digit}+
floating    [+-]?({digit}+\.{digit}+)

%%

{floating}   { float_count++; }
{integer}    { int_count++; }
[ \t\r\n]+  { /* skip whitespace */ }
.            { /* skip everything else */ }

%%

int main(void)
{
    printf("Enter the input (Ctrl+D to end):\n");
    yylex();
    printf("\nIntegers = %d\n", int_count);
    printf("Floating point numbers = %d\n", float_count);
    return 0;
}