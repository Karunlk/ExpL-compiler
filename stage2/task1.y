%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "task1.h"
    #include "task1.c"

    int yylex(void);
    void yyerror(const char *s);

    tnode *root = NULL;
%}

%token BEGIN_TOKEN
%token END_TOKEN
%token READ
%token WRITE
%token <str> ID
%token <val> NUM

/* Operators */
%left '+'
%left '-'
%left '*'
%left '/'

%union {
    int val;
    char *str;
    struct tnode *node;
}

%type <node> program
%type <node> slist
%type <node> stmt
%type <node> inputstmt
%type <node> outputstmt
%type <node> asgstmt
%type <node> expr


%%

program: BEGIN_TOKEN slist END_TOKEN ';'
            {
              root = createTree(0, 0, "PROGRAM", $2, NULL);
              root->nodetype = NODE_PROGRAM;
              $$ = root;
            }

        | BEGIN_TOKEN END_TOKEN ';'
            {
              root = createTree(0, 0, "PROGRAM", NULL, NULL);
              root->nodetype = NODE_PROGRAM;
              $$ = root;
            }
        ;


slist: slist stmt
            {
                $$ = createTree(0, 0, NULL, $1, $2);
                $$->nodetype = NODE_CONNECTOR;
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
    ;


inputstmt: READ '(' ID ')' ';'
            {
              $$ = createTree(0, TYPE_INT, NULL,
                          createTree(0, TYPE_INT, $3, NULL, NULL),
                          NULL);

              $$->nodetype = NODE_READ;

              $$->left->nodetype = NODE_ID;

              free($3);
            }
        ;


outputstmt: WRITE '(' expr ')' ';'
            {
              $$ = createTree(0, TYPE_INT, NULL, $3, NULL);
              $$->nodetype = NODE_WRITE;
            }
          ;


asgstmt: ID '=' expr ';'
        {
              tnode *idnode;

              idnode = createTree(0, TYPE_INT, $1, NULL, NULL);
              idnode->nodetype = NODE_ID;

              $$ = createTree(0, TYPE_INT, NULL,
                          idnode, $3);

              $$->nodetype = NODE_ASSIGN;

              free($1);
        }
        ;


expr: expr '+' expr
      {
          $$ = createTree(0, TYPE_INT, NULL, $1, $3);
          $$->nodetype = NODE_PLUS;
      }

    | expr '-' expr
      {
          $$ = createTree(0, TYPE_INT, NULL, $1, $3);
          $$->nodetype = NODE_MINUS;
      }

    | expr '*' expr
      {
          $$ = createTree(0, TYPE_INT, NULL, $1, $3);
          $$->nodetype = NODE_MUL;
      }

    | expr '/' expr
      {
          $$ = createTree(0, TYPE_INT, NULL, $1, $3);
          $$->nodetype = NODE_DIV;
      }

    | '(' expr ')'
      {
          $$ = $2;
      }

    | NUM
      {
          $$ = createTree($1, TYPE_INT, NULL, NULL, NULL);
          $$->nodetype = NODE_NUM;
      }

    | ID
      {
          $$ = createTree(0, TYPE_INT, $1, NULL, NULL);
          $$->nodetype = NODE_ID;

          free($1);
      }
    ;

%%


void yyerror(const char *s)
{
    fprintf(stderr, "Parse error: %s\n", s);
}


int main(void)
{
    if (yyparse() == 0) {
        printf("Abstract Syntax Tree:\n");
        printTree(root, 0);

        freeTree(root);
    }
    else {
        printf("Parsing failed!\n");
    }

    return 0;
}