%{
#include <stdio.h>
#include <stdlib.h>

#include "exe2.h"
#include "exe2.c"

int yylex(void);
void yyerror(const char *s);

tnode *root = NULL;
int current_type;

static int pointerType(int type)
{
    if (type == TYPE_INT)
        return TYPE_INT_PTR;
    if (type == TYPE_STR)
        return TYPE_STR_PTR;

    printf("Error: pointers can only point to int or str variables\n");
    exit(1);
}

static int dereferencedType(int type)
{
    if (type == TYPE_INT_PTR)
        return TYPE_INT;
    if (type == TYPE_STR_PTR)
        return TYPE_STR;

    printf("Error: cannot dereference a non-pointer expression\n");
    exit(1);
}
%}

%union
{
    int val;
    char *str;
    int type;
    struct tnode *node;
}

%token BEGIN_TOKEN END_TOKEN
%token READ WRITE
%token IF THEN ELSE ENDIF
%token WHILE DO ENDWHILE
%token REPEAT UNTIL
%token BREAK CONTINUE
%token LT GT LE GE NE EQ
%token DECL ENDDECL INT STR

%token <str> ID
%token <val> NUM
%token <str> STRCONST

%left LT GT LE GE NE EQ
%left '+' '-'
%left '*' '/' '%'
%right UADDR UDERF

%type <node> program
%type <node> slist
%type <node> stmt
%type <node> inputstmt
%type <node> outputstmt
%type <node> asgstmt
%type <node> ifstmt
%type <node> whilestmt
%type <node> repeatstmt
%type <node> dowhilestmt
%type <node> breakstmt
%type <node> continuestmt
%type <node> expr

%type <type> type

%%

program : declarations BEGIN_TOKEN slist END_TOKEN ';'
        {
            $$ = createTree(0, TYPE_NONE, NULL,NODE_PROGRAM, $3, NULL, NULL);
            root = $$;
        }
        | declarations BEGIN_TOKEN END_TOKEN ';'
        {
            $$ = createTree(0, TYPE_NONE, NULL,NODE_PROGRAM, NULL, NULL, NULL);
            root = $$;
        }
        ;

slist : slist stmt
      {
          $$ = createTree(0, TYPE_NONE, NULL,NODE_CONNECTOR, $1, NULL, $2);
      }
      | stmt
      {
          $$ = $1;
      }
      ;

stmt : inputstmt
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
     | breakstmt
     {
         $$ = $1;
     }
     | continuestmt
     {
         $$ = $1;
     }
     | repeatstmt
     {
         $$ = $1;
     }
     | dowhilestmt
     {
         $$ = $1;
     }
     ;

inputstmt : READ '(' ID ')' ';'
          {
              gsymbol *entry;
              tnode *idnode;

              entry = lookup($3);

              if(entry == NULL)
              {
                  printf("Error: Variable %s not declared\n", $3);
                  exit(1);
              }
              if(entry->dimension != 0)
              {
                  printf("Error: %s is an array; an index is required\n", $3);
                  exit(1);
              }

              idnode = createTree(0,entry->type,$3,NODE_ID,NULL,NULL,NULL);

              idnode->Gentry = entry;

              $$ = createTree(0,TYPE_NONE,NULL,NODE_READ,idnode,NULL,NULL);

              free($3);
          }
          | READ '(' ID '[' expr ']' ')' ';'
          {
                gsymbol *entry;
                tnode * arraynode;

                entry = lookup($3);
                if(entry == NULL)
                {
                    printf("Error: Variable %s not declared\n",$3);
                    exit(1);
                }
                if(entry->dimension != 1)
                {
                    printf("Error: %s is not a one-dimensional array\n",$3);
                    exit(1);
                }
                if($5->type != TYPE_INT)
                {
                    printf("Error: Array index must be of type Integer\n");
                    exit(1);
                }
                arraynode = createTree(0,entry->type,$3,NODE_ARRAY,$5,NULL,NULL);
                arraynode->Gentry = entry;
                $$ = createTree(0,TYPE_NONE,NULL,NODE_READ,arraynode,NULL,NULL);
                free($3);
          }
          | READ '(' ID '[' expr ']' '[' expr ']' ')' ';'
          {
                gsymbol *entry = lookup($3);
                tnode *arraynode;

                if (entry == NULL)
                {
                    printf("Error: Variable %s not declared\n", $3);
                    exit(1);
                }
                if (entry->dimension != 2)
                {
                    printf("Error: %s is not a two-dimensional array\n", $3);
                    exit(1);
                }
                if ($5->type != TYPE_INT || $8->type != TYPE_INT)
                {
                    printf("Error: Array indices must be of type Integer\n");
                    exit(1);
                }

                arraynode = createTree(0, entry->type, $3, NODE_2DARRAY, $5, $8, NULL);
                arraynode->Gentry = entry;
                $$ = createTree(0, TYPE_NONE, NULL, NODE_READ, arraynode, NULL, NULL);
                free($3);
          }
          | READ '(' '*' expr ')' ';'
          {
                tnode *derefnode = createTree(0, dereferencedType($4->type),
                                               NULL, NODE_DEREF, $4, NULL, NULL);
                $$ = createTree(0, TYPE_NONE, NULL, NODE_READ,
                                derefnode, NULL, NULL);
          }
          ;

outputstmt : WRITE '(' expr ')' ';'
           {
               $$ = createTree(0,TYPE_NONE,NULL,NODE_WRITE,$3,NULL,NULL);
           }
           ;

asgstmt : ID '=' expr ';'
        {
            gsymbol *entry;
            tnode *idnode;

            entry = lookup($1);

            if(entry == NULL)
            {
                printf("Error: Variable %s not declared\n", $1);
                exit(1);
            }
            if(entry->dimension != 0)
            {
                printf("Error: %s is an array; an index is required\n", $1);
                exit(1);
            }
            idnode = createTree(0,entry->type,$1,NODE_ID,NULL,NULL,NULL);
            idnode->Gentry = entry;
            $$ = createTree(0,TYPE_NONE,NULL,NODE_ASSIGN,idnode,NULL,$3);
            free($1);
        }
        | ID '[' expr ']' '=' expr ';'
        {
            gsymbol * entry;
            tnode* arraynode;

            entry = lookup($1);
            if(entry == NULL)
            {
                printf("Error: Variable %s not declared\n",$1);
                exit(1);
            }
            if(entry->dimension != 1)
            {
                printf("Error: %s is not a one-dimensional array\n",$1);
                exit(1);
            }
            if($3->type != TYPE_INT)
            {
                printf("Error: Array index must be of type Integer\n");
                exit(1);
            }
            if(entry->type != $6->type)
            {
                printf("Error: Type mismatch in assignment to array %s\n",$1);
                exit(1);
            }
            arraynode = createTree(0,entry->type,$1,NODE_ARRAY,$3,NULL,NULL);
            arraynode->Gentry = entry;
            $$ = createTree(0,TYPE_NONE,NULL,NODE_ASSIGN,arraynode,NULL,$6);
            free($1);
        }
        | ID '[' expr ']' '[' expr ']' '=' expr ';'
        {
            gsymbol * entry;
            tnode* arraynode;

            entry = lookup($1);
            if(entry == NULL)
            {
                printf("Error: Variable %s not declared\n",$1);
                exit(1);
            }
            if(entry->dimension != 2)
            {
                printf("Error: %s is not a two-dimensional array\n",$1);
                exit(1);
            }
            if($3->type != TYPE_INT || $6->type != TYPE_INT)
            {
                printf("Error: Array index must be of type Integer\n");
                exit(1);
            }
            if(entry->type != $9->type)
            {
                printf("Error: Type mismatch in assignment to array %s\n",$1);
                exit(1);
            }
            arraynode = createTree(0,entry->type,$1,NODE_2DARRAY,$3,$6,NULL);
            arraynode->Gentry = entry;
            $$ = createTree(0,TYPE_NONE,NULL,NODE_ASSIGN,arraynode,NULL,$9);
            free($1);
        }
        | '*' expr '=' expr ';'
        {
            tnode *derefnode = createTree(0, dereferencedType($2->type),
                                           NULL, NODE_DEREF, $2, NULL, NULL);
            $$ = createTree(0, TYPE_NONE, NULL, NODE_ASSIGN,
                            derefnode, NULL, $4);
        }
        ;

ifstmt : IF '(' expr ')' THEN slist ELSE slist ENDIF ';'
       {
           $$ = createTree(0,TYPE_NONE,NULL,NODE_IF,$3,$6,$8);
       }
       | IF '(' expr ')' THEN slist ENDIF ';'
       {
           $$ = createTree(0,TYPE_NONE,NULL,NODE_IF,$3,$6,NULL);
       }
       ;

whilestmt : WHILE '(' expr ')' DO slist ENDWHILE ';'
          {
              $$ = createTree(0,TYPE_NONE,NULL,NODE_WHILE,$3,$6,NULL);
          }
          ;

repeatstmt : REPEAT slist UNTIL '(' expr ')' ';'
           {
               $$ = createTree(0,TYPE_NONE,NULL,NODE_REPEAT,$5,$2,NULL);
           }
           ;

dowhilestmt : DO slist WHILE '(' expr ')' ';'
            {
                $$ = createTree(0,TYPE_NONE,NULL,NODE_DOWHILE,$5,$2,NULL);
            }
            ;

breakstmt : BREAK ';'
          {
              $$ = createTree(0,TYPE_NONE,NULL,NODE_BREAK,NULL,NULL,NULL);
          }
          ;

continuestmt : CONTINUE ';'
             {
                 $$ = createTree(0,TYPE_NONE,NULL,NODE_CONTINUE,NULL,NULL,NULL);
             }
             ;

declarations : DECL decllist ENDDECL
             {
             }
             | DECL ENDDECL
             {
             }
             ;

decllist : decllist decl
         {
         }
         | decl
         {
         }
         ;

decl : type varlist ';'
     {
     }
     ;

type : INT
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

varlist : varlist ',' ID
        {
            install($3, current_type, 1,0,0,0);
            free($3);
        }
        | varlist ',' '*' ID
        {
            install($4, pointerType(current_type), 1,0,0,0);
            free($4);
        }
        | varlist ',' ID '[' NUM ']'
        {
            if ($5 <= 0) { printf("Error: Array size must be positive\n"); exit(1); }
            install($3, current_type, $5,$5,0,1);
            free($3);
        }
        | varlist ',' ID '[' NUM ']' '[' NUM ']'
        {
            if ($5 <= 0 || $8 <= 0) { printf("Error: Array dimensions must be positive\n"); exit(1); }
            install($3, current_type, $5 * $8,$5,$8,2);
            free($3);
        }
        | ID
        {
            install($1, current_type, 1,0,0,0);
            free($1);
        }
        | ID '[' NUM ']'
        {
            if ($3 <= 0) { printf("Error: Array size must be positive\n"); exit(1); }
            install($1, current_type, $3,$3,0,1);
            free($1);
        }
        | ID '[' NUM ']' '[' NUM ']'
        {
            if ($3 <= 0 || $6 <= 0) { printf("Error: Array dimensions must be positive\n"); exit(1); }
            install($1, current_type, $3 * $6,$3,$6,2);
            free($1);
        }
        | '*' ID
        {
            install($2, pointerType(current_type), 1,0,0,0);
            free($2);
        }
        ;

expr : expr '+' expr
     {
         $$ = createTree(0,TYPE_INT,NULL,NODE_PLUS,$1,NULL,$3);
     }
     | expr '-' expr
     {
         $$ = createTree(0,TYPE_INT,NULL,NODE_MINUS,$1,NULL,$3);
     }
     | expr '*' expr
     {
         $$ = createTree(0,TYPE_INT,NULL,NODE_MUL,$1,NULL,$3);
     }
     | expr '/' expr
     {
         $$ = createTree(0,TYPE_INT,NULL,NODE_DIV,$1,NULL,$3);
     }
     | expr '%' expr
     {
         $$ = createTree(0,TYPE_INT,NULL,NODE_MOD,$1,NULL,$3);
     }
     | expr LT expr
     {
         $$ = createTree(0,TYPE_BOOL,NULL,NODE_LT,$1,NULL,$3);
     }
     | expr GT expr
     {
         $$ = createTree(0,TYPE_BOOL,NULL,NODE_GT,$1,NULL,$3);
     }
     | expr LE expr
     {
         $$ = createTree(0,TYPE_BOOL,NULL,NODE_LE,$1,NULL,$3);
     }
     | expr GE expr
     {
         $$ = createTree(0,TYPE_BOOL,NULL,NODE_GE,$1,NULL,$3);
     }
     | expr NE expr
     {
         $$ = createTree(0,TYPE_BOOL,NULL,NODE_NE,$1,NULL,$3);
     }
     | expr EQ expr
     {
         $$ = createTree(0,TYPE_BOOL,NULL,NODE_EQ,$1,NULL,$3);
     }
     | '(' expr ')'
     {
         $$ = $2;
     }
     | NUM
     {
         $$ = createTree($1,TYPE_INT,NULL,NODE_NUM,NULL,NULL,NULL);
     }
     | ID
     {
         gsymbol *entry;

         entry = lookup($1);

         if(entry == NULL)
         {
             printf("Error: Variable %s not declared\n", $1);
             exit(1);
         }
         if(entry->dimension != 0)
         {
             printf("Error: %s is an array; an index is required\n", $1);
             exit(1);
         }

         $$ = createTree(0,entry->type,$1,NODE_ID,NULL,NULL,NULL);

         $$->Gentry = entry;

         free($1);
     }
     | ID '[' expr ']'
     {
        gsymbol * entry;
        entry = lookup($1);
        if(entry == NULL)
        {
            printf("Error: Variable %s not declared\n",$1);
            exit(1);
        }
        if(entry->dimension != 1)
        {
            printf("Error: %s is not a one-dimensional array\n",$1);
            exit(1);
        }
        if($3->type != TYPE_INT)
        {
            printf("Error: Array index must be of type Integer\n");
            exit(1);
        }
        $$ = createTree(0,entry->type,$1,NODE_ARRAY,$3,NULL,NULL);
        $$->Gentry = entry;
        free($1);
     }
     | ID '[' expr ']' '[' expr ']'
     {
        gsymbol *entry;
        entry = lookup($1);
        if(entry == NULL)
        {
            printf("Error: Variable %s is not declared\n",$1);
            exit(1);
        }
        if(entry->dimension != 2)
        {
            printf("Error: %s is not a two-dimensional array\n",$1);
            exit(1);
        }
        if($3->type != TYPE_INT || $6->type != TYPE_INT)
        {
            printf("Error: Array index must be of type Integer\n");
            exit(1);
        }
        $$ = createTree(0,entry->type,$1,NODE_2DARRAY,$3,$6,NULL);
        $$->Gentry = entry;
        free($1);
     }
     | '&' ID %prec UADDR
     {
        gsymbol *entry = lookup($2);

        if (entry == NULL)
        {
            printf("Error: Variable %s not declared\n", $2);
            exit(1);
        }
        if (entry->dimension != 0)
        {
            printf("Error: %s is an array; an index is required\n", $2);
            exit(1);
        }

        $$ = createTree(0, pointerType(entry->type), $2, NODE_ADDRESS,
                        NULL, NULL, NULL);
        $$->Gentry = entry;
        free($2);
     }
     | '*' expr %prec UDERF
     {
        $$ = createTree(0, dereferencedType($2->type), NULL, NODE_DEREF,
                        $2, NULL, NULL);
     }
     | STRCONST
     {
            $$ = createTree(0,TYPE_STR,$1,NODE_STRCONST,NULL,NULL,NULL);
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

    if(argc != 2)
    {
        printf("Usage: %s <input-file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    target_file = fopen("output.o","w");

    if(yyin == NULL)
    {
        perror("Cannot open input file");
        return 1;
    }

    if(yyparse() == 0)
    {
        fprintf(target_file,
                            "0\n"
                            "2056\n"
                            "0\n"
                            "0\n"
                            "0\n"
                            "0\n"
                            "0\n"
                            "0\n");

        fprintf(target_file,"BRKP\n");
        fprintf(target_file,"MOV SP, %d\n",getSP());
        codeGen(root,target_file);
        fprintf(target_file,"INT 10\n");
        printf("Object file generated as output.o\n");
    }
    else
    {
        printf("Parsing failed!\n");
    }

    fclose(target_file);
    fclose(yyin);
    freeTree(root);
    freeGsymbol();
    return 0;
}
