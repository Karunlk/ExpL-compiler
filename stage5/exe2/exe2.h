#ifndef EXE2_H
#define EXE2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_BOOL 0
#define TYPE_INT 1
#define TYPE_STR 2
#define TYPE_NONE -1
#define TYPE_INT_PTR 3
#define TYPE_STR_PTR 4
#define TYPE_TUPLE 5
#define TYPE_TUPLE_PTR 6

#define NODE_NUM 1
#define NODE_STRCONST 2
#define NODE_ID 3
#define NODE_PLUS 4
#define NODE_MINUS 5
#define NODE_MUL 6
#define NODE_DIV 7
#define NODE_MOD 8
#define NODE_LT 9
#define NODE_GT 10
#define NODE_LE 11
#define NODE_GE 12
#define NODE_NE 13
#define NODE_EQ 14
#define NODE_ASSIGN 15
#define NODE_READ 16
#define NODE_WRITE 17
#define NODE_IF 18
#define NODE_CONNECTOR 19
#define NODE_RETURN 20
#define NODE_FUNCALL 21
#define NODE_ARRAY 22
#define NODE_WHILE 23
#define NODE_AND 24
#define NODE_ADDRESS 25
#define NODE_DEREF 26
#define NODE_FIELD 27

struct Paramstruct;
typedef struct Paramstruct {
    char *name;
    int type;
    struct Paramstruct *next;
} Paramstruct;

typedef struct TupleField {
    char *name;
    int type;
    int offset;
    struct TupleField *next;
} TupleField;

typedef struct TupleDef {
    char *name;
    int size;
    TupleField *fields;
    struct TupleDef *next;
} TupleDef;

typedef struct Gsymbol {
    char *name;
    int type;
    int size;
    int binding;
    int dimension;
    TupleDef *tuple;
    Paramstruct *paramlist;
    int flabel;
    struct Gsymbol *next;
} Gsymbol;

typedef struct Lsymbol {
    char *name;
    int type;
    int binding;
    TupleDef *tuple;
    struct Lsymbol *next;
} Lsymbol;

typedef struct tnode {
    int nodetype;
    int type;
    int val;
    char *varname;
    Gsymbol *gentry;
    Lsymbol *lentry;
    struct tnode *left;
    struct tnode *middle;
    struct tnode *right;
    struct tnode *next;
} tnode;

typedef struct FunctionAst {
    char *name;
    tnode *tree;
    Lsymbol *locals;
    struct FunctionAst *next;
} FunctionAst;

extern Gsymbol *Ghead;
extern Lsymbol *Lhead;
extern FunctionAst *functions;
extern TupleDef *Tuples;

Gsymbol *lookupGlobal(const char *name);
void installGlobal(const char *name, int type, Paramstruct *params);
void installGlobalVariable(const char *name, int type, int size);
void installTupleVariable(const char *name, TupleDef *tuple);
void installTuplePointerVariable(const char *name, TupleDef *tuple);
TupleDef *installTuple(const char *name, TupleField *fields);
TupleField *appendTupleField(TupleField *head, TupleField *field);
TupleField *findField(TupleDef *tuple, const char *name);
Paramstruct *makeParam(const char *name, int type);
Paramstruct *appendParam(Paramstruct *head, Paramstruct *param);
void freeParams(Paramstruct *params);
void checkSignature(Gsymbol *function, int type, Paramstruct *params);

Lsymbol *lookupLocal(const char *name);
int lookupVisible(const char *name, Gsymbol **global, Lsymbol **local);
void beginFunctionScope(Paramstruct *params);
void installLocal(const char *name, int type);
void installLocalTuple(const char *name, TupleDef *tuple);
void installLocalTuplePointer(const char *name, TupleDef *tuple);
void endFunctionScope(void);
void freeLocalTable(Lsymbol *locals);

void printGlobals(void);
void printLocals(void);
void printAst(tnode *tree, int level);
void freeAst(tnode *tree);
void freeAll(void);
void generateProgram(const char *filename);

tnode *makeNode(int nodetype, int type, int val, const char *name,
                tnode *left, tnode *middle, tnode *right);
tnode *makeIdentifier(const char *name);
tnode *makeFunctionCall(Gsymbol *function, tnode *args);
void semanticError(const char *message, const char *name);

#endif
