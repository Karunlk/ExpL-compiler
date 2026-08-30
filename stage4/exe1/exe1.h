#ifndef EXE1_H
#define EXE1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NODE_NUM        1
#define NODE_ID         2

#define NODE_PLUS       3
#define NODE_MINUS      4
#define NODE_MUL        5
#define NODE_DIV        6

#define NODE_ASSIGN     7
#define NODE_READ       8
#define NODE_WRITE      9

#define NODE_CONNECTOR  10
#define NODE_PROGRAM    11

#define NODE_LT         12
#define NODE_GT         13
#define NODE_LE         14
#define NODE_GE         15
#define NODE_NE         16
#define NODE_EQ         17

#define NODE_IF         18
#define NODE_WHILE      19
#define NODE_BREAK      20
#define NODE_CONTINUE   21
#define NODE_REPEAT     22
#define NODE_DOWHILE    23

#define NODE_STRCONST   24
#define NODE_ARRAY      25
#define NODE_2DARRAY    26

#define TYPE_BOOL        0
#define TYPE_INT         1
#define TYPE_STR         2
#define TYPE_NONE       -1

typedef struct Gsymbol
{
    char* name;
    int type;
    int size;
    int rows;
    int columns;
    int dimension;
    int binding;
    struct Gsymbol *next;
}gsymbol;

typedef struct tnode
{
    int val;
    int type;
    char *varname;
    int nodetype;
    struct Gsymbol *Gentry;
    struct tnode *left;
    struct tnode *middle;
    struct tnode *right;

} tnode;

tnode* createTree(int val, int type, char *c,int nodetype,tnode *l, tnode *m, tnode *r);
void freeTree(tnode *root);
void printTree(tnode *root, int level);

int codeGen(tnode *t, FILE *target_file);
int GetLabel(void);
int getReg(void);
void freeReg(int reg);

int getAddress(gsymbol *entry);
gsymbol* lookup(char* name);
void install(char* name, int type, int size,int rows, int columns, int dimension);
void printGsymbol(void);
void freeGsymbol(void);

extern gsymbol *Ghead;

int getSP(void);

#endif