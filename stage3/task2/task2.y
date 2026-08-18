%{
#include <stdio.h>
#include <stdlib.h>
#include "task2.h"
#include "task2.c"

int yylex(void);
void yyerror(const char *s);
tnode *root = NULL;
%}

%union
{
    int val;
    char *str;
    struct tnode *node;
}

%token BEGIN_TOKEN END_TOKEN
%token READ WRITE
%token IF THEN ELSE ENDIF
%token WHILE DO ENDWHILE
%token LT GT LE GE NE EQ

%token <str> ID
%token <val> NUM

%left LT GT LE GE NE EQ
%left '+' '-'
%left '*' '/'

%type <node> program
%type <node> slist
%type <node> stmt
%type <node> inputstmt
%type <node> outputstmt
%type <node> asgstmt
%type <node> ifstmt
%type <node> whilestmt
%type <node> expr

%%

program: BEGIN_TOKEN slist END_TOKEN ';'
    {
        $$ = createTree(0, TYPE_NONE, NULL, NODE_PROGRAM, $2, NULL, NULL);
        root = $$;
    }
    | BEGIN_TOKEN END_TOKEN ';'
    {
        $$ = createTree(0, TYPE_NONE, NULL, NODE_PROGRAM, NULL, NULL, NULL);
        root = $$;
    }
    ;

slist: slist stmt
    {
        $$ = createTree(0, TYPE_NONE, NULL, NODE_CONNECTOR, $1, NULL, $2);
    }
    | stmt
    {
        $$ = $1;
    }
    ;

stmt: inputstmt
    {
        $$ = $1;
    }
    | outputstmt
    {
        $$ = $1;
    }
    | asgstmt
    {
        $$ = $1;
    }
    | ifstmt
    {
        $$ = $1;
    }
    | whilestmt
    {
        $$ = $1;
    }
    ;

inputstmt: READ '(' ID ')' ';'
         {
             tnode *idnode;

             idnode = createTree(0, TYPE_INT, $3, NODE_ID, NULL, NULL, NULL);

             $$ = createTree(0, TYPE_NONE, NULL, NODE_READ, idnode, NULL, NULL);

             free($3);
         }
         ;

outputstmt: WRITE '(' expr ')' ';'
          {
              $$ = createTree(0, TYPE_NONE, NULL, NODE_WRITE, $3, NULL, NULL);
          }
          ;

asgstmt: ID '=' expr ';'
       {
          tnode *idnode;

          idnode = createTree(0, TYPE_INT, $1, NODE_ID, NULL, NULL, NULL);

          $$ = createTree(0, TYPE_NONE, NULL, NODE_ASSIGN, idnode, NULL, $3);

          free($1);
       }
       ;

ifstmt: IF '(' expr ')' THEN slist ELSE slist ENDIF ';'
      {
         $$ = createTree(0, TYPE_NONE, NULL, NODE_IF, $3, $6, $8);
      }
      | IF '(' expr ')' THEN slist ENDIF ';'
      {
         $$ = createTree(0, TYPE_NONE, NULL, NODE_IF, $3, $6, NULL);
      }
      ;

whilestmt: WHILE '(' expr ')' DO slist ENDWHILE ';'
         {
             $$ = createTree(0, TYPE_NONE, NULL, NODE_WHILE, $3, $6, NULL);
         }
         ;

expr: expr '+' expr
    {
        $$ = createTree(0, TYPE_INT, NULL, NODE_PLUS, $1, NULL, $3);
    }
    | expr '-' expr
    {
        $$ = createTree(0, TYPE_INT, NULL, NODE_MINUS, $1, NULL, $3);
    }
    | expr '*' expr
    {
        $$ = createTree(0, TYPE_INT, NULL, NODE_MUL, $1, NULL, $3);
    }
    | expr '/' expr
    {
        $$ = createTree(0, TYPE_INT, NULL, NODE_DIV, $1, NULL, $3);
    }
    | expr LT expr
    {
        $$ = createTree(0, TYPE_BOOL, NULL, NODE_LT, $1, NULL, $3);
    }
    | expr GT expr
    {
        $$ = createTree(0, TYPE_BOOL, NULL, NODE_GT, $1, NULL, $3);
    }
    | expr LE expr
    {
        $$ = createTree(0, TYPE_BOOL, NULL, NODE_LE, $1, NULL, $3);
    }
    | expr GE expr
    {
        $$ = createTree(0, TYPE_BOOL, NULL, NODE_GE, $1, NULL, $3);
    }
    | expr NE expr
    {
        $$ = createTree(0, TYPE_BOOL, NULL, NODE_NE, $1, NULL, $3);
    }
    | expr EQ expr
    {
        $$ = createTree(0, TYPE_BOOL, NULL, NODE_EQ, $1, NULL, $3);
    }
    | '(' expr ')'
    {
        $$ = $2;
    }
    | NUM
    {
        $$ = createTree($1, TYPE_INT, NULL, NODE_NUM, NULL, NULL, NULL);
    }
    | ID
    {
        $$ = createTree(0, TYPE_INT, $1, NODE_ID, NULL, NULL, NULL);
        free($1);
    }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "Parse error: %s\n", s);
}

int main(int argc, char *argv[])
{
    extern FILE *yyin;
    FILE *target_file;

    if (argc != 2)
    {
        printf("Usage: %s <input-file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");

    if (yyin == NULL)
    {
        perror("Cannot open input file");
        return 1;
    }

    target_file = fopen("output.o", "w");

    if (target_file == NULL)
    {
        perror("Cannot open output file");
        fclose(yyin);
        return 1;
    }

    if (yyparse() == 0)
    {
        fprintf(target_file,"0\n""2056\n""0\n""0\n""0\n""0\n""0\n""0\n");
        fprintf(target_file,"BRKP\n");
        fprintf(target_file,"MOV SP, 4121\n");
        codeGen(root, target_file);
        fprintf(target_file,"INT 10\n");
        printf("\nXSM object code generated successfully in output.o\n");
        freeTree(root);
    }
    else
    {
        printf("Parsing failed!\n");
    }

    fclose(target_file);
    fclose(yyin);

    return 0;
}