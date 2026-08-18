#ifndef TASK4_H
#define TASK4_H

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

#define TYPE_BOOL        0
#define TYPE_INT         1
#define TYPE_NONE       -1


typedef struct tnode
{
    int val;
    int type;
    char *varname;
    int nodetype;

    struct tnode *left;
    struct tnode *middle;
    struct tnode *right;

} tnode;
tnode* createTree(int val, int type, char *c,int nodetype,tnode *l, tnode *m, tnode *r);
void freeTree(tnode *root);
int codeGen(tnode *t, FILE *target_file);
int GetLabel(void);
int getReg(void);
void freeReg(int reg);
int getAddress(char *name);

#endif