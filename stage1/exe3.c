#include <stdio.h>
#include <stdlib.h>
#include "exe2.h"

static int reg_free[20] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

int getReg(void)
    {
        for(int i=0;i<20;i++)
        {
            if(reg_free[i]==1)
            {
                reg_free[i]=0;
                return i;
            }
        }
        exit(1);
    }

void freeReg(void)
    {
        for(int i=19;i>=0;i--)
        {
            if(reg_free[i]==0)
            {
                reg_free[i]=1;
                return;
            }
        }
        return;
    }

tnode* makeLeafNode(int n){
    tnode *temp = (tnode*)malloc(sizeof(tnode));
    if(!temp) { perror("malloc"); exit(1); }
    temp->op = '\0';
    temp->val = n;
    temp->left = temp->right = NULL;
    return temp;
}

tnode* makeOperatorNode(char c, tnode *l, tnode *r){
    tnode *temp = (tnode*)malloc(sizeof(tnode));
    if(!temp) { perror("malloc"); exit(1); }
    temp->op = c;
    temp->left = l;
    temp->right = r;
    return temp;
}

int codeGen(struct tnode * t, FILE * target_file)
    {
        if(t->left == NULL && t->right == NULL)
        {
            int reg = getReg();
            fprintf(target_file, "MOV R%d, %d\n", reg, t->val);
            return reg;
        }
        else
        {
            int i = codeGen(t->left, target_file);
            int j = codeGen(t->right,target_file);
            switch(t->op)
            {
                case '+':
                    fprintf(target_file, "ADD R%d, R%d\n", i, j);
                    freeReg();
                    return i;
                    break;
                case '-':
                    fprintf(target_file, "SUB R%d, R%d\n", i, j);
                    freeReg();
                    return i;
                    break;
                case '*':
                    fprintf(target_file, "MUL R%d, R%d\n", i, j);
                    freeReg();
                    return i;
                    break;
                case '/':
                    fprintf(target_file, "DIV R%d, R%d\n", i, j);
                    freeReg();
                    return i;
                    break;
            }
            freeReg();
            return i;
        }
    }