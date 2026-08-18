%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "task2.c"
    #include "task2.h"

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

%token BEGIN_TOKEN END_TOKEN READ WRITE
%token <val> NUM
%token <str> ID
%type <node> program slist stmt inputstmt outputstmt asgstmt expr
%left '+' '-'
%left '*' '/'

%%
program: BEGIN_TOKEN slist END_TOKEN ';'
      {
          $$ = createTree(0,TYPE_INT,NULL,$2,NULL);
          $$->nodetype = NODE_PROGRAM;
          root = $$;
      }

    | BEGIN_TOKEN END_TOKEN ';'
      {
          $$ = createTree(0,TYPE_INT,NULL,NULL,NULL);
          $$->nodetype = NODE_PROGRAM;
          root = $$;
      }
    ;

slist: slist stmt
      {
          $$ = createTree(0,TYPE_INT,NULL,$1,$2);
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
          tnode *idnode;
          idnode = createTree(0,TYPE_INT,$3,NULL,NULL);
          idnode->nodetype = NODE_ID;
          $$ = createTree(0,TYPE_INT,NULL,idnode,NULL);
          $$->nodetype = NODE_READ;
          free($3);
      }
    ;
outputstmt: WRITE '(' expr ')' ';'
            {
              $$ = createTree(0,TYPE_INT,NULL,$3,NULL);
              $$->nodetype = NODE_WRITE;
            }
          ;
asgstmt: ID '=' expr ';'
        {
          tnode *idnode;

          idnode = createTree(0,TYPE_INT,$1,NULL,NULL);
          idnode->nodetype = NODE_ID;
          $$ = createTree(0,TYPE_INT,NULL,idnode,$3);
          $$->nodetype = NODE_ASSIGN;
          free($1);
        }
       ;
expr: expr '+' expr
      {
          $$ = createTree(0,TYPE_INT,NULL,$1,$3);
          $$->nodetype = NODE_PLUS;
      }
    | expr '-' expr
      {
          $$ = createTree(0,TYPE_INT,NULL,$1,$3);
          $$->nodetype = NODE_MINUS;
      }
    | expr '*' expr
      {
          $$ = createTree(0,TYPE_INT,NULL,$1,$3);
          $$->nodetype = NODE_MUL;
      }
    | expr '/' expr
      {
          $$ = createTree(0,TYPE_INT,NULL,$1,$3);
          $$->nodetype = NODE_DIV;
      }
    | '(' expr ')'
      {
          $$ = $2;
      }
    | NUM
      {
          $$ = createTree($1,TYPE_INT,NULL,NULL,NULL);
          $$->nodetype = NODE_NUM;
      }
    | ID
      {
          $$ = createTree(0,TYPE_INT,$1,NULL,NULL);
          $$->nodetype = NODE_ID;
          free($1);
      }
    ;
%%


void yyerror(const char *s)
{
    fprintf(
        stderr,
        "Parse error: %s\n",
        s
    );
}

int main(int argc, char *argv[])
{
    FILE *target_file;
    extern FILE *yyin;
    if (argc != 2)
    {
        printf("Usage: %s <input-file>\n",argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");

    if (yyin == NULL)
    {
        perror("Cannot open input file");
        return 1;
    }
    if (yyparse() != 0)
    {
        printf("Parsing failed\n");
        fclose(yyin);
        return 1;
    }
    fclose(yyin);
    printf("\nParsing successful!\n\n");

    target_file = fopen("output.xsm", "w");

    if (target_file == NULL)
    {
        perror("Cannot create output.xsm");
        freeTree(root);
        return 1;
    }
    fprintf(target_file,"0\n""2056\n""0\n""0\n""0\n""0\n""0\n""0\n");
    fprintf(target_file,"BRKP\n");
    fprintf(target_file,"MOV SP, 4121\n");
    codeGen(root, target_file);
    fprintf(target_file,"INT 10\n");
    fclose(target_file);
    printf("\nXSM code generated successfully ""in output.xsm\n");
    freeTree(root);
    return 0;
}