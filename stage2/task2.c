#include "task2.h"
static int reg_free[20] = {1, 1, 1, 1, 1,1, 1, 1, 1, 1,1, 1, 1, 1, 1,1, 1, 1, 1, 1};

int getReg(void)
{
    int i;
    for (i = 0; i < 20; i++)
    {
        if (reg_free[i] == 1)
        {
            reg_free[i] = 0;
            return i;
        }
    }
    printf("Error: No free registers\n");
    exit(1);
}

void freeReg(int reg)
{
    if (reg >= 0 && reg < 20)
        reg_free[reg] = 1;
}

int getAddress(char *varname)
{
    return 4096 + (varname[0] - 'a');
}

tnode* createTree(int val,int type,char *c,tnode *l,tnode *r)
{
    tnode *temp = (tnode *)malloc(sizeof(tnode));
    if (temp == NULL)
    {
        perror("malloc");
        exit(1);
    }
    temp->val = val;
    temp->type = type;
    if (c != NULL)
        temp->varname = strdup(c);
    else
        temp->varname = NULL;
    temp->left = l;
    temp->right = r;
    temp->nodetype = 0;
    return temp;
}

void freeTree(tnode *root)
{
    if (root == NULL)
        return;
    freeTree(root->left);
    freeTree(root->right);
    if (root->varname != NULL)
        free(root->varname);
    free(root);
}

int codeGen(tnode *t, FILE *target_file)
{
    int i, j;
    int reg;
    int address;
    if (t == NULL)
        return -1;

    if (t->nodetype == NODE_NUM)
    {
        reg = getReg();
        fprintf(target_file,"MOV R%d, %d\n",reg,t->val);
        return reg;
    }

    if (t->nodetype == NODE_ID)
    {
        reg = getReg();
        address = getAddress(t->varname);
        fprintf(target_file,"MOV R%d, [%d]\n",reg,address);
        return reg;
    }

    if (t->nodetype == NODE_PLUS)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);
        fprintf(target_file,"ADD R%d, R%d\n",i,j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_MINUS)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);
        fprintf(target_file,"SUB R%d, R%d\n",i,j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_MUL)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);
        fprintf(target_file,"MUL R%d, R%d\n",i,j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_DIV)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);
        fprintf(target_file,"DIV R%d, R%d\n",i,j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_ASSIGN)
    {
        i = codeGen(t->right, target_file);
        address = getAddress(t->left->varname);
        fprintf(target_file,"MOV [%d], R%d\n",address,i);
        freeReg(i);
        return -1;
    }

    if (t->nodetype == NODE_READ)
    {
        address = getAddress(t->left->varname);
        reg = getReg();
        fprintf(target_file,"MOV R%d, \"Read\"\n",reg);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"MOV R%d, -1\n",reg);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"MOV R%d, %d\n",reg,address);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"CALL 0\n");
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        freeReg(reg);
        return -1;
    }

    if (t->nodetype == NODE_WRITE)
    {
        i = codeGen(t->left, target_file);
        reg = getReg();
        fprintf(target_file,"MOV R%d, \"Write\"\n",reg);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"MOV R%d, -2\n",reg);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"PUSH R%d\n",i);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"PUSH R%d\n",reg);
        fprintf(target_file,"CALL 0\n");
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        fprintf(target_file,"POP R%d\n",reg);
        freeReg(reg);
        freeReg(i);

        return -1;
    }

    if (t->nodetype == NODE_CONNECTOR)
    {
        codeGen(t->left, target_file);
        codeGen(t->right, target_file);
        return -1;
    }

    if (t->nodetype == NODE_PROGRAM)
    {
        codeGen(t->left, target_file);
        return -1;
    }
    printf(
        "Error: Unknown node type %d\n",
        t->nodetype
    );
    exit(1);
}