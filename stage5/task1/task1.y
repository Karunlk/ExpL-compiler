%{
#include <stdio.h>
#include <stdlib.h>

#include "task1.h"
#include "task1.c"

int yylex(void);
void yyerror(const char *message);

static int current_type;
%}

%union {
    int val;
    char *str;
    int type;
    Paramstruct *param;
}

%token DECL ENDDECL INT STR
%token <str> ID
%token <val> NUM
%token <str> STRCONST

%type <type> type
%type <param> paramlist param

%%

program
    : declarations function_list
    ;

function_list
    : /* empty */
    | function_list function_definition
    ;

function_definition
    : type ID '(' paramlist ')' block
      {
          free($2);
      }
    ;

block
    : '{' block_items '}'
    ;

block_items
    : /* empty */
    | block_items block_item
    ;

block_item
    : block
    | ID
    | NUM
    | STRCONST
    | DECL
    | ENDDECL
    | INT
    | STR
    | '['
    | ']'
    | '('
    | ')'
    | ';'
    | ','
    | '*'
    | '+'
    | '-'
    | '/'
    | '%'
    | '='
    | '<'
    | '>'
    | '&'
    ;

declarations
    : DECL declaration_list ENDDECL
    | DECL ENDDECL
    ;

declaration_list
    : declaration_list declaration
    | declaration
    ;

declaration
    : type ID '(' paramlist ')' ';'
      {
          installFunction($2, $1, $4);
          free($2);
      }
    | type variable_list ';'
    ;

paramlist
    : /* empty */
      {
          $$ = NULL;
      }
    | param
      {
          $$ = $1;
      }
    | paramlist ',' param
      {
          $$ = appendParam($1, $3);
      }
    ;

param
    : type ID
      {
          $$ = makeParam($2, $1);
          free($2);
      }
    ;

type
    : INT
      {
          current_type = TYPE_INT;
          $$ = TYPE_INT;
      }
    | STR
      {
          current_type = TYPE_STR;
          $$ = TYPE_STR;
      }
    ;

variable_list
    : variable_list ',' variable
    | variable
    ;

variable
    : ID
      {
          installVariable($1, current_type, 1, 0, 0, 0);
          free($1);
      }
    | '*' ID
      {
          int pointerType = (current_type == TYPE_INT) ? TYPE_INT_PTR : TYPE_STR_PTR;
          installVariable($2, pointerType, 1, 0, 0, 0);
          free($2);
      }
    | ID '[' NUM ']'
      {
          if ($3 <= 0)
          {
              fprintf(stderr, "Error: array size must be positive\n");
              exit(EXIT_FAILURE);
          }
          installVariable($1, current_type, $3, $3, 0, 1);
          free($1);
      }
    | ID '[' NUM ']' '[' NUM ']'
      {
          if ($3 <= 0 || $6 <= 0)
          {
              fprintf(stderr, "Error: array dimensions must be positive\n");
              exit(EXIT_FAILURE);
          }
          installVariable($1, current_type, $3 * $6, $3, $6, 2);
          free($1);
      }
    ;

%%

void yyerror(const char *message)
{
    fprintf(stderr, "Parse error: %s\n", message);
}

int main(int argc, char **argv)
{
    extern FILE *yyin;
    int result;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL)
    {
        perror("Cannot open input file");
        return EXIT_FAILURE;
    }

    result = yyparse();
    if (result == 0)
        printGsymbol();

    fclose(yyin);
    freeGsymbol();
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
