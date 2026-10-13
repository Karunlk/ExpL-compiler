#ifndef EXPTREE_H
#define EXPTREE_H
#include <stdio.h>

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Type Constants */
#define INTEGER_TYPE 101
#define BOOLEAN_TYPE 102
#define STRING_TYPE 103
#define INTEGER_POINTER_TYPE 104
#define INTEGER_ARRAY_TYPE 105
#define STRING_ARRAY_TYPE 106
#define STRING_POINTER_TYPE 107
#define USER_ARRAY_TYPE 110     /* array of a user-defined type (element type in Gsymbol.elem) */
#define NULL_TYPE 108            /* type of the constant NULL */
#define ALLOC_TYPE 109           /* type of an alloc() call: assignable to any user-defined type */
#define USER_TYPE_BASE 200       /* user-defined type i has type code USER_TYPE_BASE + i */
#define isUserType(t) ((t) >= USER_TYPE_BASE)
#define MAX_TYPES 32
#define MAX_FIELDS 8             /* fixed heap block size is 8 words */

/* Member field of a user-defined type */
typedef struct Field {
    char* name;
    int type;         // type code of the field
    int index;        // word offset inside the heap block
    struct Field* next;
} Field;

/* Type table: one entry per user-defined type (int/str/NULL are the implicit entries) */
typedef struct Typetable {
    char* name;
    int code;
    int nfields;
    Field* fields;
} Typetable;

/* Formal parameter list of a function */
typedef struct Paramstruct {
    char* name;
    int type;
    struct Paramstruct* next;
} Paramstruct;

/* Global symbol table: global variables and functions */
typedef struct Gsymbol {
    char* name;       // name of the variable / function
    int type;         // type of the variable / return type of the function
    int size;         // size of the variable (0 for functions)
    int* metadata;    // array dimensions: NULL for scalar, [size] for 1D, [rows, cols] for 2D
    int binding;      // static memory address allocated to the variable (-1 for functions)
    Paramstruct* paramlist; // formal parameters (functions only)
    int flabel;       // function label number: F<flabel> (-1 for variables)
    int isFunction;   // 1 if this entry is a function
    int defined;      // 1 once the function definition has been seen
    int elem;         // element type of an array of user-defined type (0 otherwise)
    struct Gsymbol *next;
} Gsymbol;

/* Local symbol table: formal parameters and local variables of one function */
typedef struct Lsymbol {
    char* name;
    int type;
    int binding;      // offset relative to BP (params: -3,-4,..  locals: 1,2,..)
    struct Lsymbol* next;
} Lsymbol;

/* Global symbol table */
Gsymbol* Lookup(char* name);
int Install(char* name, int type, int size, int* metadata);
int InstallFunction(char* name, int type, Paramstruct* paramlist);
void print_symbol_table();
int get_stack_pointer();
Gsymbol* get_global_head();

/* Parameter lists */
Paramstruct* makeParam(char* name, int type);
Paramstruct* appendParam(Paramstruct* list, Paramstruct* p);
int countParams(Paramstruct* list);

/* Local symbol table (one active table at a time: the function being parsed) */
void LReset();
Lsymbol* LLookup(char* name);
int LInstallParams(Paramstruct* params);
int LInstall(char* name, int type);
int LLocalCount();
void print_local_table(char* fname);

/* Type table */
int TCreate(char* name);                 // returns type code, -1 if the name is already a type
int TLookup(char* name);                 // type code, -1 if undefined
int TAddField(int code, char* name, int type); // 0 ok, 1 duplicate field, 2 too many fields
Field* FLookup(int code, char* name);
int TCount();
const char* typeName(int type);
void print_type_table();

#endif

/* Node Type Constants */
#define PROGRAM 0
#define STATEMENT 1
#define EXPRESSION 2
#define VARIABLE 3
#define INPUT 4
#define OUTPUT 5
#define ASSIGNMENT 6
#define PLUS_NODE 7
#define MINUS_NODE 8
#define MUL_NODE 9
#define DIV_NODE 10
#define CONSTANT 11
#define READ_NODE 13
#define WRITE_NODE 14
#define EQ_NODE 15
#define NE_NODE 16
#define GT_NODE 17
#define LT_NODE 18
#define GE_NODE 19
#define LE_NODE 20
#define IF_NODE 21
#define WHILE_NODE 22
#define BREAK_NODE 23
#define CONTINUE_NODE 24
#define DO_WHILE_NODE 25
#define REPEAT_UNTIL_NODE 26
#define STRING_CONSTANT_NODE 27
#define ADDRESS_NODE 28
#define DEREF_NODE 29
#define MODULUS_NODE 30
#define FUNC_CALL_NODE 31   // varname = function name, Gentry = function entry, left = argument list
#define ARG_NODE 32         // left = argument expression, right = next ARG_NODE
#define RETURN_NODE 33      // left = returned expression
#define AND_NODE 34
#define OR_NODE 35
#define NOT_NODE 36
#define FIELD_NODE 37       // left = base (VARIABLE or FIELD_NODE), val = field offset, varname = field name
#define ALLOC_NODE 38       // alloc()
#define FREE_NODE 39        // free(left)
#define INIT_NODE 40        // initialize()



typedef struct tnode {
    int val;        // value of a number for NUM nodes.
    int type;       // type of the expression represented by this node
    char* varname;  // name of a variable / function for ID nodes
    int nodetype;   // information about non-leaf nodes - read/write/connector/+/* etc.
    Gsymbol* Gentry; // global symbol table entry (global variables and functions)
    Lsymbol* Lentry; // local symbol table entry (locals and parameters)
    struct tnode *left,*right,*mid;
} tnode;

/*Create a node tnode*/
tnode* createNode(int val, int type, int nodetype, char* c, struct tnode *l, struct tnode *r, struct tnode* m);

/* Variable reference: resolves name in local table first, then global table */
tnode* makeVarNode(char* name, tnode* index1, tnode* index2);

/* Member field access: base.field */
tnode* makeFieldNode(tnode* base, char* field);

/* Function call node with argument list (chain of ARG_NODEs) */
tnode* makeCallNode(char* name, tnode* args);

int codeGen(tnode *t, FILE *target_file);

int getLabel();

int getReg();

void freeReg();

/* Code generation driver */
void setCurrentFunction(Gsymbol* f, int returnType);
int getCurrentReturnType();
void genStartup(FILE* fp);
void genFunction(char* label, int nlocals, tnode* body, FILE* fp);

int pointerTo(int base_type);

#endif
