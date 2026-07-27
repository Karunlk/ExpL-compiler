%option noyywrap

%{
    #include<stdio.h>
    #include<stdlib.h>
    int chars = 0; 
    int words = 0; 
    int lines = 0;
    int has_content = 0;
    int last_was_newline = 1;
%}



%%
\n              { lines++; chars++; has_content = 1; last_was_newline = 1; }
[ \t]+          { chars += yyleng; has_content = 1; last_was_newline = 0; }
[^ \t\n]+       { words++; chars += yyleng; has_content = 1; last_was_newline = 0; }
<<EOF>>         { if(has_content && !last_was_newline)
                    {
                        lines++;
                    }
                    return 0;
                }
%%

int main()
{
    yylex();

    printf("Number of Lines = %d\n", lines);
    printf("Number of Words = %d\n", words);
    printf("Number of Characters = %d\n", chars);
    
    return 0;
}