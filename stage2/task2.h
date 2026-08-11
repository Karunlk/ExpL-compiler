#ifndef TASK2_H
#define TASK2_H

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

typedef struct tnode
{
    int val;                    /* Value for NUM */
    int type;                   /* Data type */
    char *varname;              /* Variable name for ID */
    int nodetype;               /* Type of AST node */
    struct tnode *left,*right; /* Left and right branches */
}tnode;

tnode* createTree(int val,int type,char *c,tnode *l,tnode *r);

void printTree(tnode *root, int level);

void freeTree(tnode *root);

int codeGen(tnode *t, FILE *target_file);

int getReg(void);

void freeReg(int reg);

int getAddress(char *varname);

#endif