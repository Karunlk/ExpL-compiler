#include "task3.h"
static int reg_free[20] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

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
int labelCount = 0;
int GetLabel(void)
{
    return labelCount++;
}

tnode* createTree(int val, int type, char *c,int nodetype,tnode *l, tnode *m, tnode *r)
{
    tnode *temp;
    if (nodetype == NODE_PLUS ||nodetype == NODE_MINUS ||nodetype == NODE_MUL ||nodetype == NODE_DIV)
    {
        if (l->type != TYPE_INT || r->type != TYPE_INT)
        {
            printf("Type mismatch: arithmetic operator requires integer operands\n");
            exit(1);
        }

        type = TYPE_INT;
    }

    else if (nodetype == NODE_LT ||nodetype == NODE_GT ||nodetype == NODE_LE ||nodetype == NODE_GE ||nodetype == NODE_NE ||nodetype == NODE_EQ)
    {
        if (l->type != TYPE_INT || r->type != TYPE_INT)
        {
            printf("Type mismatch: relational operator requires integer operands\n");
            exit(1);
        }

        type = TYPE_BOOL;
    }

    else if (nodetype == NODE_ASSIGN)
    {
        if (l->type != TYPE_INT || r->type != TYPE_INT)
        {
            printf("Type mismatch: assignment requires integer expression\n");
            exit(1);
        }

        type = TYPE_NONE;
    }

    else if (nodetype == NODE_WRITE)
    {
        if (l->type != TYPE_INT)
        {
            printf("Type mismatch: write requires integer expression\n");
            exit(1);
        }

        type = TYPE_NONE;
    }

    else if (nodetype == NODE_IF)
    {
        if (l->type != TYPE_BOOL)
        {
            printf("Type mismatch: if condition must be boolean\n");
            exit(1);
        }

        type = TYPE_NONE;
    }

    else if (nodetype == NODE_WHILE)
    {
        if (l->type != TYPE_BOOL)
        {
            printf("Type mismatch: while condition must be boolean\n");
            exit(1);
        }

        type = TYPE_NONE;
    }


    temp = (tnode *)malloc(sizeof(tnode));

    if (temp == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    temp->val = val;
    temp->type = type;
    temp->nodetype = nodetype;
    temp->left = l;
    temp->middle = m;
    temp->right = r;

    if (c != NULL)
        temp->varname = strdup(c);
    else
        temp->varname = NULL;
    return temp;
}

int codeGen(tnode *t, FILE *target_file)
{
    int i, j;
    int reg;
    int address;
    int label1, label2;

    if (t == NULL)
        return -1;

    if (t->nodetype == NODE_NUM)
    {
        reg = getReg();
        fprintf(target_file,"MOV R%d, %d\n",reg, t->val);
        return reg;
    }

    if (t->nodetype == NODE_ID)
    {
        reg = getReg();
        address = getAddress(t->varname);
        fprintf(target_file,"MOV R%d, [%d]\n",reg, address);
        return reg;
    }

    if (t->nodetype == NODE_PLUS)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"ADD R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_MINUS)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"SUB R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_MUL)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"MUL R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_DIV)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"DIV R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_LT)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"LT R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_GT)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"GT R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_LE)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"LE R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_GE)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"GE R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_NE)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"NE R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_EQ)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file,"EQ R%d, R%d\n",i, j);
        freeReg(j);
        return i;
    }

    if (t->nodetype == NODE_ASSIGN)
    {
        i = codeGen(t->right, target_file);

        address = getAddress(t->left->varname);

        fprintf(target_file,"MOV [%d], R%d\n",address, i);
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

        fprintf(target_file,"MOV R%d, %d\n",reg, address);

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

    if (t->nodetype == NODE_WRITE){
    i = codeGen(t->left, target_file);
    reg = getReg();

    fprintf(target_file, "MOV R%d, \"Write\"\n", reg);
    fprintf(target_file, "PUSH R%d\n", reg);
    fprintf(target_file, "MOV R%d, -2\n", reg);
    fprintf(target_file, "PUSH R%d\n", reg);
    fprintf(target_file, "PUSH R%d\n", i);
    fprintf(target_file, "PUSH R%d\n", reg);
    fprintf(target_file, "PUSH R%d\n", reg);
    fprintf(target_file, "CALL 0\n");

    fprintf(target_file, "POP R%d\n", reg);
    fprintf(target_file, "POP R%d\n", reg);
    fprintf(target_file, "POP R%d\n", reg);
    fprintf(target_file, "POP R%d\n", reg);
    fprintf(target_file, "POP R%d\n", reg);

    freeReg(reg);
    freeReg(i);

    return -1;
}

if (t->nodetype == NODE_IF)
{
    i = codeGen(t->left, target_file);

    if (t->right != NULL)
    {
        label1 = GetLabel();
        label2 = GetLabel();

        fprintf(target_file, "JZ R%d, L%d\n", i, label1);
        freeReg(i);

        codeGen(t->middle, target_file);

        fprintf(target_file, "JMP L%d\n", label2);
        fprintf(target_file, "L%d:\n", label1);

        codeGen(t->right, target_file);

        fprintf(target_file, "L%d:\n", label2);
    }
    else
    {
        label1 = GetLabel();

        fprintf(target_file, "JZ R%d, L%d\n", i, label1);
        freeReg(i);

        codeGen(t->middle, target_file);

        fprintf(target_file, "L%d:\n", label1);
    }

    return -1;
}

if (t->nodetype == NODE_WHILE)
{
    label1 = GetLabel();
    label2 = GetLabel();

    fprintf(target_file, "L%d:\n", label1);

    i = codeGen(t->left, target_file);

    fprintf(target_file, "JZ R%d, L%d\n", i, label2);
    freeReg(i);

    codeGen(t->middle, target_file);

    fprintf(target_file, "JMP L%d\n", label1);
    fprintf(target_file, "L%d:\n", label2);

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

printf("Error: Unknown node type %d\n", t->nodetype);
exit(1);
}
void freeTree(tnode *root)
{
    if (root == NULL)
        return;
    freeTree(root->left);
    freeTree(root->middle);
    freeTree(root->right);
    if (root->varname != NULL)
        free(root->varname);
    free(root);
}