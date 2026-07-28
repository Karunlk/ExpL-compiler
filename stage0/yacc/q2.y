%{
#include <stdio.h>
#include <ctype.h>

int yylex();
void yyerror(const char *s);
%}

%token ID

%%

S : ID
    {
        printf("Valid Variable\n");
    }
  ;

%%
int yylex()
{
    static int end = 1;
    static char str[100];

    if (!end)
    {
        end = 1;
        return 0;      // Tell yyparse this input is finished
    }

    if (fgets(str, sizeof(str), stdin) == NULL)
        return 0;

    end = 0;

    if (!isalpha(str[0]))
        return '?';

    for (int i = 0; str[i] != '\n' && str[i] != '\0'; i++)
    {
        if (!isalnum(str[i]))
            return '?';
    }

    return ID;
}

void yyerror(const char *s)
{
    printf("Invalid Variable\n");
}

int main()
{
        printf("Enter a variable (Ctrl+D to exit): ");
        yyparse();

    return 0;
}