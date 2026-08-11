%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "exe1.h"
    #include "exe1.c"

    int yylex(void);
    void yyerror(const char *s);
    tnode *root = NULL;
%}

%union {
    int val;
    char *str;
    struct tnode *node;
}

%token BEGIN_TOKEN END_TOKEN READ WRITE
%token <str> ID
%token <val> NUM
%left '+' '-'
%left '*' '/'
%type <node> program slist stmt inputstmt outputstmt asgstmt expr

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
            $$ = createTree(0,TYPE_INT,NULL,createTree(0,TYPE_INT,$3,NULL, NULL),NULL);

            $$->nodetype = NODE_READ;
            $$->left->nodetype = NODE_ID;

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
    fprintf(stderr, "Parse error: %s\n", s);
}


int main(int argc, char *argv[])
{
    extern FILE *yyin;

    if (argc != 2)
    {
        printf("Usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");

    if (yyin == NULL)
    {
        printf("Cannot open source file\n");
        return 1;
    }

    if (yyparse() == 0)
    {
        printf("Enter input values for READ statements in %s:\n", argv[1]);
        evaluate(root);

        freeTree(root);
    }
    else
    {
        printf("Parsing failed!\n");
    }

    fclose(yyin);

    return 0;
}