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

#define TYPE_INT        1

typedef struct tnode {
    int val;                  /* value for NUM */
    int type;                 /* variable type */
    char *varname;            /* variable name for ID */
    int nodetype;             /* type of node */
    struct tnode *left;
    struct tnode *right;
} tnode;

tnode* createTree(int val, int type, char *c, tnode *l, tnode *r);

void freeTree(tnode *root);

int evaluate(tnode *root);

#endif