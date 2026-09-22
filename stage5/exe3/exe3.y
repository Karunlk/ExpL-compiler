%{
#include <stdio.h>
#include <stdlib.h>
#include "exe3.h"
#include "exe3.c"
int yylex(void);
void yyerror(const char *message);
static int currentType;
static int declaredType;
static int currentReturnType;
static int currentHasReturn;
static Gsymbol *currentFunction;
static TupleDef *currentReturnTuple;

static int pointerType(int type)
{
  if (type == TYPE_INT) return TYPE_INT_PTR;
  if (type == TYPE_STR) return TYPE_STR_PTR;
  if (type == TYPE_TUPLE) return TYPE_TUPLE_PTR;
  semanticError("cannot create pointer to this type", NULL);
  return TYPE_NONE;
}

static int dereferencedType(int type)
{
  if (type == TYPE_INT_PTR) return TYPE_INT;
  if (type == TYPE_STR_PTR) return TYPE_STR;
  semanticError("cannot dereference non-pointer expression", NULL);
  return TYPE_NONE;
}
%}

%union { int val; char *str; int type; void *param; tnode *node; TupleDef *tuple; }
%token DECL ENDDECL BEGIN_TOKEN END_TOKEN READ WRITE IF THEN ELSE ENDIF RETURN WHILE DO ENDWHILE AND
%token INT STR LT GT LE GE NE EQ
%token TUPLE
%token <str> ID STRCONST
%token <val> NUM
%type <type> type
%type <type> return_type
%type <tuple> tuple_type
%type <param> paramlist param
%type <param> fieldlist field tuplevars
%type <node> expr arglist stmt stmtlist stmtseq function_body
%left LT GT LE GE NE EQ
%left AND
%left '+' '-'
%left '*' '/' '%'
%right UADDR UDERF

%%
program : declarations function_definitions { printGlobals(); }
;
declarations : DECL declaration_list ENDDECL | DECL ENDDECL | /* empty */
;
declaration_list : declaration_list declaration | declaration
;
declaration : type { declaredType = $1; } declaration_items ';'
            | tuple_type ID '(' paramlist ')' ';'
              { installGlobalTupleFunction($2, TYPE_TUPLE, $1, (Paramstruct *)$4); free($2); }
            | tuple_type '*' ID '(' paramlist ')' ';'
              { installGlobalTupleFunction($3, TYPE_TUPLE_PTR, $1, (Paramstruct *)$5); free($3); }
            | TUPLE ID '(' fieldlist ')' tuplevars ';'
              { TupleDef *tuple = installTuple($2, (TupleField *)$4); Paramstruct *var = (Paramstruct *)$6; while (var) { if (var->type == TYPE_TUPLE_PTR) installTuplePointerVariable(var->name, tuple); else installTupleVariable(var->name, tuple); var = var->next; } freeParams((Paramstruct *)$6); free($2); }
;
declaration_items : declaration_items ',' declaration_item | declaration_item
;
declaration_item : ID
                   { installGlobalVariable($1, declaredType, 1); free($1); }
                 | ID '[' NUM ']'
                   { if ($3 <= 0) semanticError("array size must be positive", NULL); installGlobalVariable($1, declaredType, $3); free($1); }
                 | ID '(' paramlist ')'
                   { installGlobal($1, declaredType, (Paramstruct *)$3); free($1); }
                 | '*' ID
                   { installGlobalVariable($2, pointerType(declaredType), 1); free($2); }
                 | '*' ID '(' paramlist ')'
                   { installGlobal($2, pointerType(declaredType), (Paramstruct *)$4); free($2); }
;
paramlist : /* empty */ { $$ = NULL; }
          | param { $$ = $1; }
          | paramlist ',' param { $$ = appendParam((Paramstruct *)$1, (Paramstruct *)$3); }
;
fieldlist : field { $$ = $1; }
          | fieldlist ',' field { $$ = appendTupleField((TupleField *)$1, (TupleField *)$3); }
;
field : type ID { TupleField *field = calloc(1, sizeof(*field)); field->name = strdup($2); field->type = $1; free($2); $$ = (Paramstruct *)field; }
;
tuplevars : ID { $$ = makeParam($1, TYPE_NONE); free($1); }
          | '*' ID { $$ = makeParam($2, TYPE_TUPLE_PTR); free($2); }
          | tuplevars ',' ID { $$ = appendParam((Paramstruct *)$1, makeParam($3, TYPE_NONE)); free($3); }
          | tuplevars ',' '*' ID { $$ = appendParam((Paramstruct *)$1, makeParam($4, TYPE_TUPLE_PTR)); free($4); }
;
param : type ID { $$ = makeParam($2, $1); free($2); }
  | type '*' ID { $$ = makeParam($3, pointerType($1)); free($3); }
  | tuple_type ID { $$ = makeTupleParam($2, TYPE_TUPLE, $1); free($2); }
  | tuple_type '*' ID { $$ = makeTupleParam($3, TYPE_TUPLE_PTR, $1); free($3); }
;
type : INT { currentType = TYPE_INT; $$ = TYPE_INT; }
     | STR { currentType = TYPE_STR; $$ = TYPE_STR; }
;
return_type : type { declaredType = $1; $$ = $1; }
;
tuple_type : ID
             { $$ = lookupTuple($1); if (!$$) semanticError("tuple must be declared before use", $1); free($1); }
;
function_definitions : /* empty */ | function_definitions function_definition
;
function_definition
  : return_type ID '(' paramlist ')' '{'
      {
          FunctionAst *defined;
          currentFunction = lookupGlobal($2);
          if (currentFunction) checkSignature(currentFunction, $1, (Paramstruct *)$4);
          else if (strcmp($2, "main") != 0) semanticError("function has no declaration", $2);
          if (strcmp($2, "main") == 0 && ($1 != TYPE_INT || $4 != NULL)) semanticError("main must be int main()", NULL);
          for (defined = functions; defined; defined = defined->next)
            if (strcmp(defined->name, $2) == 0) semanticError("duplicate function definition:", $2);
          currentReturnType = $1; currentHasReturn = 0;
          beginFunctionScope((Paramstruct *)$4); freeParams((Paramstruct *)$4);
      }
        function_body
      {
          FunctionAst *function = calloc(1, sizeof(*function));
          if (!currentHasReturn) semanticError("function has no return statement", $2);
          function->name = strdup($2); function->tree = $8; function->locals = Lhead;
          function->next = functions; functions = function; Lhead = NULL; free($2);
      }
  | tuple_type ID '(' paramlist ')' '{'
      {
          FunctionAst *defined; currentFunction = lookupGlobal($2);
          if (currentFunction) checkSignature(currentFunction, TYPE_TUPLE, (Paramstruct *)$4);
          else semanticError("function has no declaration", $2);
          for (defined = functions; defined; defined = defined->next)
            if (strcmp(defined->name, $2) == 0) semanticError("duplicate function definition:", $2);
          currentReturnType = TYPE_TUPLE; currentReturnTuple = $1; currentHasReturn = 0;
          beginFunctionScope((Paramstruct *)$4); freeParams((Paramstruct *)$4);
      }
            function_body
          {
            FunctionAst *function = calloc(1, sizeof(*function));
            if (!currentHasReturn) semanticError("function has no return statement", $2);
              function->name = strdup($2); function->tree = $8; function->locals = Lhead;
            function->next = functions; functions = function; Lhead = NULL; free($2);
          }
  | tuple_type '*' ID '(' paramlist ')' '{'
      {
          FunctionAst *defined; currentFunction = lookupGlobal($3);
          if (currentFunction) checkSignature(currentFunction, TYPE_TUPLE_PTR, (Paramstruct *)$5);
          else semanticError("function has no declaration", $3);
          for (defined = functions; defined; defined = defined->next)
            if (strcmp(defined->name, $3) == 0) semanticError("duplicate function definition:", $3);
          currentReturnType = TYPE_TUPLE_PTR; currentReturnTuple = $1; currentHasReturn = 0;
          beginFunctionScope((Paramstruct *)$5); freeParams((Paramstruct *)$5);
      }
        function_body
      {
          FunctionAst *function = calloc(1, sizeof(*function));
          if (!currentHasReturn) semanticError("function has no return statement", $3);
          function->name = strdup($3); function->tree = $9; function->locals = Lhead;
          function->next = functions; functions = function; Lhead = NULL; free($3);
      }
  | type '*' ID '(' paramlist ')' '{'
          {
            FunctionAst *defined;
            currentFunction = lookupGlobal($3);
            if (currentFunction) checkSignature(currentFunction, pointerType($1), (Paramstruct *)$5);
            else semanticError("function has no declaration", $3);
            for (defined = functions; defined; defined = defined->next)
            if (strcmp(defined->name, $3) == 0)
              semanticError("duplicate function definition:", $3);
            currentReturnType = pointerType($1);
            currentHasReturn = 0;
            beginFunctionScope((Paramstruct *)$5);
            freeParams((Paramstruct *)$5);
          }
          function_body
          {
            FunctionAst *function = calloc(1, sizeof(*function));
            if (!currentHasReturn) semanticError("function has no return statement", $3);
            function->name = strdup($3);
            function->tree = $9;
            function->locals = Lhead;
            function->next = functions;
            functions = function;
            Lhead = NULL;
            free($3);
          }
;
function_body : local_declarations BEGIN_TOKEN stmtlist END_TOKEN '}' { $$ = $3; }
;
local_declarations : DECL local_declaration_list ENDDECL | DECL ENDDECL | /* empty */
;
local_declaration_list : local_declaration_list local_declaration | local_declaration
;
local_declaration : type local_variable_list ';'
                  | TUPLE ID '(' fieldlist ')' tuplevars ';'
                    { TupleDef *tuple = installTuple($2, (TupleField *)$4); Paramstruct *var = (Paramstruct *)$6; while (var) { if (var->type == TYPE_TUPLE_PTR) installLocalTuplePointer(var->name, tuple); else installLocalTuple(var->name, tuple); var = var->next; } freeParams((Paramstruct *)$6); free($2); }
;
local_variable_list : local_variable_list ',' local_variable | local_variable
;
local_variable : ID { installLocal($1, currentType); free($1); }
               | '*' ID { installLocal($2, pointerType(currentType)); free($2); }
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
           if (id->type == TYPE_TUPLE) {
               TupleDef *leftTuple = id->lentry ? id->lentry->tuple : id->gentry->tuple;
               TupleDef *rightTuple = $3->lentry ? $3->lentry->tuple : $3->gentry ? $3->gentry->tuple : NULL;
               if (($3->nodetype != NODE_ID && $3->nodetype != NODE_FUNCALL) || !rightTuple || strcmp(leftTuple->name, rightTuple->name) != 0)
                   semanticError("tuple assignment requires matching tuple types", $1);
           }
           $$ = makeNode(NODE_ASSIGN, TYPE_NONE, 0, NULL, id, NULL, $3); free($1);
       }
     | ID '.' ID '=' expr ';'
       {
           Gsymbol *global = lookupGlobal($1); Lsymbol *local = lookupLocal($1); TupleDef *tuple = local ? local->tuple : global ? global->tuple : NULL; TupleField *field = findField(tuple, $3);
           if (!field) semanticError("invalid tuple field", $3); if ($5->type != field->type) semanticError("tuple field assignment type mismatch", $3);
           tnode *base = makeNode(NODE_FIELD, field->type, field->offset, $1, NULL, NULL, NULL); base->gentry = global; base->lentry = local; base->varname = strdup($3);
           $$ = makeNode(NODE_ASSIGN, TYPE_NONE, 0, NULL, base, NULL, $5); free($1); free($3);
       }
     | ID '(' arglist ')' ';'
       {
           Gsymbol *function = lookupGlobal($1);
           tnode *call;
           if (!function || function->flabel < 0) semanticError("undeclared function", $1);
           call = makeFunctionCall(function, $3);
           $$ = call;
           free($1);
       }
     | ID '[' expr ']' '=' expr ';'
       {
           Gsymbol *entry = lookupGlobal($1);
           tnode *array;
           if (!entry || entry->dimension != 1) semanticError("invalid array", $1);
           if ($3->type != TYPE_INT) semanticError("array index must be integer", $1);
           if (entry->type != $6->type) semanticError("array assignment type mismatch", $1);
           array = makeNode(NODE_ARRAY, entry->type, 0, $1, $3, NULL, NULL);
           array->gentry = entry;
           $$ = makeNode(NODE_ASSIGN, TYPE_NONE, 0, NULL, array, NULL, $6);
           free($1);
       }
     | READ '(' ID ')' ';'
       { tnode *id = makeIdentifier($3); if (id->gentry && id->gentry->flabel >= 0) semanticError("cannot read into function", $3); $$ = makeNode(NODE_READ, TYPE_NONE, 0, NULL, id, NULL, NULL); free($3); }
     | READ '(' ID '[' expr ']' ')' ';'
       {
           Gsymbol *entry = lookupGlobal($3);
           tnode *array;
           if (!entry || entry->dimension != 1) semanticError("invalid array", $3);
           if ($5->type != TYPE_INT) semanticError("array index must be integer", $3);
           array = makeNode(NODE_ARRAY, entry->type, 0, $3, $5, NULL, NULL);
           array->gentry = entry;
           $$ = makeNode(NODE_READ, TYPE_NONE, 0, NULL, array, NULL, NULL);
           free($3);
       }
     | READ '(' ID '.' ID ')' ';'
       { Gsymbol *global = lookupGlobal($3); Lsymbol *local = lookupLocal($3); TupleField *field = findField(local ? local->tuple : global ? global->tuple : NULL, $5); if (!field) semanticError("invalid tuple field", $5); tnode *base = makeNode(NODE_FIELD, field->type, field->offset, $3, NULL, NULL, NULL); base->gentry = global; base->lentry = local; base->varname = strdup($5); $$ = makeNode(NODE_READ, TYPE_NONE, 0, NULL, base, NULL, NULL); free($3); free($5); }
     | READ '(' '*' expr ')' ';'
       { tnode *deref = makeNode(NODE_DEREF, dereferencedType($4->type), 0, NULL, $4, NULL, NULL); $$ = makeNode(NODE_READ, TYPE_NONE, 0, NULL, deref, NULL, NULL); }
     | '*' expr '=' expr ';'
       { tnode *deref = makeNode(NODE_DEREF, dereferencedType($2->type), 0, NULL, $2, NULL, NULL); if (deref->type != $4->type) semanticError("dereference assignment type mismatch", NULL); $$ = makeNode(NODE_ASSIGN, TYPE_NONE, 0, NULL, deref, NULL, $4); }
     | WRITE '(' expr ')' ';'
       { if ($3->type != TYPE_INT && $3->type != TYPE_STR) semanticError("write requires int or str expression", NULL); $$ = makeNode(NODE_WRITE, TYPE_NONE, 0, NULL, $3, NULL, NULL); }
     | RETURN expr ';'
       { if ($2->type != currentReturnType || ((currentReturnType == TYPE_TUPLE || currentReturnType == TYPE_TUPLE_PTR) && (!currentReturnTuple || !($2->lentry ? $2->lentry->tuple : $2->gentry ? $2->gentry->tuple : NULL) || currentReturnTuple != ($2->lentry ? $2->lentry->tuple : $2->gentry->tuple)))) semanticError("return type mismatch in", currentFunction ? currentFunction->name : "main"); currentHasReturn = 1; $$ = makeNode(NODE_RETURN, TYPE_NONE, 0, NULL, $2, NULL, NULL); }
     | IF '(' expr ')' THEN stmtlist ELSE stmtlist ENDIF ';'
       { if ($3->type != TYPE_BOOL) semanticError("if condition must be boolean", NULL); $$ = makeNode(NODE_IF, TYPE_NONE, 0, NULL, $3, $6, $8); }
     | IF '(' expr ')' THEN stmtlist ENDIF ';'
       { if ($3->type != TYPE_BOOL) semanticError("if condition must be boolean", NULL); $$ = makeNode(NODE_IF, TYPE_NONE, 0, NULL, $3, $6, NULL); }
     | WHILE '(' expr ')' DO stmtlist ENDWHILE ';'
       { if ($3->type != TYPE_BOOL) semanticError("while condition must be boolean", NULL); $$ = makeNode(NODE_WHILE, TYPE_NONE, 0, NULL, $3, $6, NULL); }
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
    | expr AND expr { if ($1->type != TYPE_BOOL || $3->type != TYPE_BOOL) semanticError("AND requires boolean operands", NULL); $$ = makeNode(NODE_AND, TYPE_BOOL, 0, NULL, $1, NULL, $3); }
     | '(' expr ')' { $$ = $2; }
     | NUM { $$ = makeNode(NODE_NUM, TYPE_INT, $1, NULL, NULL, NULL, NULL); }
     | STRCONST { $$ = makeNode(NODE_STRCONST, TYPE_STR, 0, $1, NULL, NULL, NULL); free($1); }
     | ID '(' arglist ')' { Gsymbol *f = lookupGlobal($1); if (!f || f->flabel < 0) semanticError("undeclared function", $1); $$ = makeFunctionCall(f, $3); free($1); }
     | ID '[' expr ']'
       { Gsymbol *entry = lookupGlobal($1); if (!entry || entry->dimension != 1) semanticError("invalid array", $1); if ($3->type != TYPE_INT) semanticError("array index must be integer", $1); $$ = makeNode(NODE_ARRAY, entry->type, 0, $1, $3, NULL, NULL); $$->gentry = entry; free($1); }
     | ID '.' ID
       { Gsymbol *global = lookupGlobal($1); Lsymbol *local = lookupLocal($1); TupleField *field = findField(local ? local->tuple : global ? global->tuple : NULL, $3); if (!field) semanticError("invalid tuple field", $3); $$ = makeNode(NODE_FIELD, field->type, field->offset, $1, NULL, NULL, NULL); $$->gentry = global; $$->lentry = local; free($1); free($3); }
     | '&' ID
       { Gsymbol *global = lookupGlobal($2); Lsymbol *local = lookupLocal($2); if (!global && !local) semanticError("undeclared identifier", $2); $$ = makeNode(NODE_ADDRESS, pointerType(local ? local->type : global->type), 0, $2, NULL, NULL, NULL); $$->gentry = global; $$->lentry = local; free($2); }
     | '*' expr %prec UDERF
       { $$ = makeNode(NODE_DEREF, dereferencedType($2->type), 0, NULL, $2, NULL, NULL); }
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
        generateProgram("output.o");
    }
    fclose(yyin); freeAll();
    while (functions) { FunctionAst *next = functions->next; freeAst(functions->tree); freeLocalTable(functions->locals); free(functions->name); free(functions); functions = next; }
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
