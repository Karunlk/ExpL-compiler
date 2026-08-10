#ifndef TASK2_H
#define TASK2_H

typedef struct tnode{
    int val;
    char *op;
    struct tnode *left, *right;
}tnode;

tnode* makeLeafNode(int n);
tnode* makeOperatorNode(char c, tnode *l, tnode *r);
int codeGen(struct tnode * t, FILE * target_file);

int getReg(void);
void freeReg(void);

#endif