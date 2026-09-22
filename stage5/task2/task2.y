%{
#include <stdio.h>
#include <stdlib.h>
#include "task2.h"
#include "task2.c"
int yylex(void);
void yyerror(const char *message);
static int currentType;
static int currentReturnType;
static int currentHasReturn;
static Gsymbol *currentFunction;
static FunctionAst *functions;
%}

%union { int val; char *str; int type; void *param; tnode *node; }
%token DECL ENDDECL BEGIN_TOKEN END_TOKEN READ WRITE IF THEN ELSE ENDIF RETURN
%token INT STR LT GT LE GE NE EQ
%token <str> ID STRCONST
%token <val> NUM
%type <type> type
%type <param> paramlist param
%type <node> expr arglist stmt stmtlist stmtseq
%left LT GT LE GE NE EQ
%left '+' '-'
%left '*' '/' '%'

%%
program : declarations function_definitions { printGlobals(); }
;
declarations : DECL declaration_list ENDDECL | DECL ENDDECL | /* empty */
;
declaration_list : declaration_list declaration | declaration
;
declaration : type ID '(' paramlist ')' ';' { installGlobal($2, $1, (Paramstruct *)$4); free($2); }
             | type variable_list ';'
;
variable_list : variable_list ',' variable | variable
;
variable : ID { installGlobalVariable($1, currentType, 1); free($1); }
          | ID '[' NUM ']' { if ($3 <= 0) semanticError("array size must be positive", NULL); installGlobalVariable($1, currentType, $3); free($1); }
;
paramlist : /* empty */ { $$ = NULL; }
          | param { $$ = $1; }
          | paramlist ',' param { $$ = appendParam((Paramstruct *)$1, (Paramstruct *)$3); }
;
param : type ID { $$ = makeParam($2, $1); free($2); }
;
type : INT { currentType = TYPE_INT; $$ = TYPE_INT; }
     | STR { currentType = TYPE_STR; $$ = TYPE_STR; }
;
function_definitions : /* empty */ | function_definitions function_definition
;
function_definition
    : type ID '(' paramlist ')' '{'
      {
          FunctionAst *defined;
          currentFunction = lookupGlobal($2);
          if (currentFunction) checkSignature(currentFunction, $1, (Paramstruct *)$4);
          else if (strcmp($2, "main") != 0) semanticError("function has no declaration", $2);
          if (strcmp($2, "main") == 0 && ($1 != TYPE_INT || $4 != NULL))
            semanticError("main must be int main()", NULL);
          for (defined = functions; defined; defined = defined->next)
            if (strcmp(defined->name, $2) == 0)
              semanticError("duplicate function definition:", $2);
          currentReturnType = $1;
          currentHasReturn = 0;
          beginFunctionScope((Paramstruct *)$4);
            freeParams((Paramstruct *)$4);
      }
      local_declarations BEGIN_TOKEN stmtlist END_TOKEN '}'
      {
          FunctionAst *function = calloc(1, sizeof(*function));
          if (!currentHasReturn)
            semanticError("function has no return statement", $2);
          function->name = strdup($2);
          function->tree = $10;
          function->locals = Lhead;
          function->next = functions;
          functions = function;
          Lhead = NULL;
          free($2);
      }
;
local_declarations : DECL local_declaration_list ENDDECL | DECL ENDDECL | /* empty */
;
local_declaration_list : local_declaration_list local_declaration | local_declaration
;
local_declaration : type local_variable_list ';'
;
local_variable_list : local_variable_list ',' local_variable | local_variable
;
local_variable : ID { installLocal($1, currentType); free($1); }
;
stmtlist : /* empty */ { $$ = NULL; }
   | stmtseq { $$ = $1; }
;
stmtseq : stmtseq stmt { $$ = makeNode(NODE_CONNECTOR, TYPE_NONE, 0, NULL, $1, NULL, $2); }
  | stmt { $$ = $1; }
;
stmt : ID '=' expr ';'
       {
           tnode *id = makeIdentifier($1);
           if (id->gentry && id->gentry->flabel >= 0)
             semanticError("cannot assign to function", $1);
           if (id->type != $3->type) semanticError("assignment type mismatch for", $1);
           $$ = makeNode(NODE_ASSIGN, TYPE_NONE, 0, NULL, id, NULL, $3); free($1);
       }
     | READ '(' ID ')' ';'
       { tnode *id = makeIdentifier($3); if (id->gentry && id->gentry->flabel >= 0) semanticError("cannot read into function", $3); $$ = makeNode(NODE_READ, TYPE_NONE, 0, NULL, id, NULL, NULL); free($3); }
     | WRITE '(' expr ')' ';'
       { if ($3->type != TYPE_INT && $3->type != TYPE_STR) semanticError("write requires int or str expression", NULL); $$ = makeNode(NODE_WRITE, TYPE_NONE, 0, NULL, $3, NULL, NULL); }
     | RETURN expr ';'
       { if ($2->type != currentReturnType) semanticError("return type mismatch in", currentFunction ? currentFunction->name : "main"); currentHasReturn = 1; $$ = makeNode(NODE_RETURN, TYPE_NONE, 0, NULL, $2, NULL, NULL); }
     | IF '(' expr ')' THEN stmtlist ELSE stmtlist ENDIF ';'
       { if ($3->type != TYPE_BOOL) semanticError("if condition must be boolean", NULL); $$ = makeNode(NODE_IF, TYPE_NONE, 0, NULL, $3, $6, $8); }
     | IF '(' expr ')' THEN stmtlist ENDIF ';'
       { if ($3->type != TYPE_BOOL) semanticError("if condition must be boolean", NULL); $$ = makeNode(NODE_IF, TYPE_NONE, 0, NULL, $3, $6, NULL); }
;
expr : expr '+' expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("arithmetic requires integers", NULL); $$ = makeNode(NODE_PLUS, TYPE_INT, 0, NULL, $1, NULL, $3); }
     | expr '-' expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("arithmetic requires integers", NULL); $$ = makeNode(NODE_MINUS, TYPE_INT, 0, NULL, $1, NULL, $3); }
     | expr '*' expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("arithmetic requires integers", NULL); $$ = makeNode(NODE_MUL, TYPE_INT, 0, NULL, $1, NULL, $3); }
     | expr '/' expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("arithmetic requires integers", NULL); $$ = makeNode(NODE_DIV, TYPE_INT, 0, NULL, $1, NULL, $3); }
     | expr '%' expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("arithmetic requires integers", NULL); $$ = makeNode(NODE_MOD, TYPE_INT, 0, NULL, $1, NULL, $3); }
     | expr LT expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("comparison requires integers", NULL); $$ = makeNode(NODE_LT, TYPE_BOOL, 0, NULL, $1, NULL, $3); }
     | expr GT expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("comparison requires integers", NULL); $$ = makeNode(NODE_GT, TYPE_BOOL, 0, NULL, $1, NULL, $3); }
     | expr LE expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("comparison requires integers", NULL); $$ = makeNode(NODE_LE, TYPE_BOOL, 0, NULL, $1, NULL, $3); }
     | expr GE expr { if ($1->type != TYPE_INT || $3->type != TYPE_INT) semanticError("comparison requires integers", NULL); $$ = makeNode(NODE_GE, TYPE_BOOL, 0, NULL, $1, NULL, $3); }
     | expr NE expr { if ($1->type != $3->type) semanticError("comparison type mismatch", NULL); $$ = makeNode(NODE_NE, TYPE_BOOL, 0, NULL, $1, NULL, $3); }
     | expr EQ expr { if ($1->type != $3->type) semanticError("comparison type mismatch", NULL); $$ = makeNode(NODE_EQ, TYPE_BOOL, 0, NULL, $1, NULL, $3); }
     | '(' expr ')' { $$ = $2; }
     | NUM { $$ = makeNode(NODE_NUM, TYPE_INT, $1, NULL, NULL, NULL, NULL); }
     | STRCONST { $$ = makeNode(NODE_STRCONST, TYPE_STR, 0, $1, NULL, NULL, NULL); free($1); }
     | ID '(' arglist ')' { Gsymbol *f = lookupGlobal($1); if (!f || f->flabel < 0) semanticError("undeclared function", $1); $$ = makeFunctionCall(f, $3); free($1); }
     | ID { $$ = makeIdentifier($1); free($1); }
;
arglist : /* empty */ { $$ = NULL; }
        | expr { $$ = $1; }
        | arglist ',' expr { tnode *tail = $1; if (!tail) $$ = $3; else { while (tail->next) tail = tail->next; tail->next = $3; $$ = $1; } }
;
%%

void yyerror(const char *message) { fprintf(stderr, "Parse error: %s\n", message); }

int main(int argc, char **argv)
{
    extern FILE *yyin;
    int result;
    FunctionAst *function;
    if (argc != 2) { fprintf(stderr, "Usage: %s <input-file>\n", argv[0]); return EXIT_FAILURE; }
    yyin = fopen(argv[1], "r");
    if (!yyin) { perror("Cannot open input file"); return EXIT_FAILURE; }
    result = yyparse();
    if (result == 0) {
        for (function = functions; function; function = function->next) {
            printf("\nFunction %s local symbol table:\n", function->name);
            Lhead = function->locals; printLocals();
            printf("Function %s AST:\n", function->name); printAst(function->tree, 1);
        }
    }
    fclose(yyin); freeAll();
    while (functions) { FunctionAst *next = functions->next; freeAst(functions->tree); freeLocalTable(functions->locals); free(functions->name); free(functions); functions = next; }
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
