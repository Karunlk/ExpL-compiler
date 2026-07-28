%option noyywrap

%{
    #include <stdio.h>
	extern FILE *yyin;
%}

%%

"//"[^\n]*                 { printf("%s\n", yytext); }
"/*"([^*]|\*+[^*/])*\*+"/" { printf("%s\n", yytext); }
[ \t\r\n]+                 { /* skip whitespace */ }
.                           { /* skip everything else */ }

%%

int main(void)
{
    printf("Input is taken from input.txt file if not present error will be shown\n");
	FILE *input_file = fopen("input.txt", "r");
	if (input_file == NULL)
	{
		perror("input.txt");
		return 1;
	}
	yyin = input_file;
	yylex();
	fclose(input_file);
	return 0;
}
