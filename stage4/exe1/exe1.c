#include "exe1.h"

static int reg_free[20] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

#define MAX_LOOP_DEPTH 100

int breakLabel[MAX_LOOP_DEPTH];
int continueLabel[MAX_LOOP_DEPTH];
int loopDepth = 0;

gsymbol *Ghead = NULL;

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

int getSP(void)
{
    if(Ghead == NULL)
        return 4095;
    else
        return Ghead->binding + Ghead->size - 1;
}

gsymbol* lookup(char *name)
{
    gsymbol *temp = Ghead;

    while (temp != NULL)
    {
        if (strcmp(temp->name, name) == 0)
            return temp;

        temp = temp->next;
    }

    return NULL;
}

void install(char *name, int type, int size,int row,int column,int dimension)
{
    gsymbol *newSymbol;

    if (lookup(name) != NULL)
    {
        printf("Error: Variable %s already declared\n", name);
        exit(1);
    }

    newSymbol = (gsymbol *)malloc(sizeof(gsymbol));

    if (newSymbol == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    newSymbol->name = strdup(name);
    newSymbol->type = type;
    newSymbol->size = size;
    newSymbol->rows = row;
    newSymbol->columns = column;
    newSymbol->dimension = dimension;

    if (Ghead == NULL)
        newSymbol->binding = 4096;
    else
        newSymbol->binding = Ghead->binding + Ghead->size;

    if (newSymbol->binding + newSymbol->size - 1 > 5119)
    {
        printf("Error: Insufficient Memory\n");
        free(newSymbol->name);
        free(newSymbol);
        exit(1);
    }

    newSymbol->next = Ghead;
    Ghead = newSymbol;
}

int getAddress(gsymbol *entry)
{
    if (entry == NULL)
    {
        printf("Error: NULL symbol table entry\n");
        exit(1);
    }

    return entry->binding;
}

void printGsymbol(void)
{
    gsymbol *temp = Ghead;
    printf("\nGlobal Symbol Table\n");
    printf("Name\tType\tTotalSize\tRows\tColumns\tDimension\tBinding\n");
    while (temp != NULL)
    {
        if (temp->type == TYPE_INT)
            printf("%s\tINT\t%d\t%d\t%d\t%d\t%d\n",
                   temp->name,
                   temp->size,
                   temp->rows,
                   temp->columns,
                   temp->dimension,
                   temp->binding);

        else if (temp->type == TYPE_STR)
            printf("%s\tSTR\t%d\t%d\t%d\t%d\t%d\n",
                   temp->name,
                   temp->size,
                   temp->rows,
                   temp->columns,
                   temp->dimension,
                   temp->binding);

        temp = temp->next;
    }
}

int labelCount = 0;

int GetLabel(void)
{
    return labelCount++;
}

tnode* createTree(int val, int type, char *c, int nodetype, tnode *l, tnode *m, tnode *r)
{
    tnode *temp;

    if (nodetype == NODE_PLUS ||
        nodetype == NODE_MINUS ||
        nodetype == NODE_MUL ||
        nodetype == NODE_DIV)
    {
        if (l->type != TYPE_INT || r->type != TYPE_INT)
        {
            printf("Type mismatch: arithmetic operator requires integer operands\n");
            exit(1);
        }

        type = TYPE_INT;
    }

    else if (nodetype == NODE_LT ||
             nodetype == NODE_GT ||
             nodetype == NODE_LE ||
             nodetype == NODE_GE ||
             nodetype == NODE_NE ||
             nodetype == NODE_EQ)
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
        if (l->type != r->type)
        {
            printf("Type mismatch: assignment requires both sides to be of same data type\n");
            exit(1);
        }

        type = TYPE_NONE;
    }

    else if (nodetype == NODE_WRITE)
    {
        if (l->type != TYPE_INT && l->type != TYPE_STR)
        {
            printf("Type mismatch: invalid write expression\n");
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
    temp->Gentry = NULL;
    temp->left = l;
    temp->middle = m;
    temp->right = r;

    if (c != NULL)
        temp->varname = strdup(c);
    else
        temp->varname = NULL;

    return temp;
}

void printTree(tnode *root, int level)
{
    int i;

    if (root == NULL)
        return;

    for (i = 0; i < level; i++)
        printf("    ");

    switch (root->nodetype)
    {
        case NODE_PROGRAM:
            printf("PROGRAM\n");
            break;

        case NODE_CONNECTOR:
            printf("CONNECTOR\n");
            break;

        case NODE_NUM:
            printf("NUM(%d)\n", root->val);
            break;

        case NODE_ID:
            printf("ID(%s)", root->varname);

            if (root->Gentry != NULL)
            {
                printf(" [type=");

                if (root->Gentry->type == TYPE_INT)
                    printf("INT");
                else if (root->Gentry->type == TYPE_STR)
                    printf("STR");

                printf(", binding=%d]",
                       root->Gentry->binding);
            }

            printf("\n");
            break;

        case NODE_PLUS:
            printf("PLUS\n");
            break;

        case NODE_MINUS:
            printf("MINUS\n");
            break;

        case NODE_MUL:
            printf("MUL\n");
            break;

        case NODE_DIV:
            printf("DIV\n");
            break;

        case NODE_ASSIGN:
            printf("ASSIGN\n");
            break;

        case NODE_READ:
            printf("READ\n");
            break;

        case NODE_WRITE:
            printf("WRITE\n");
            break;

        case NODE_LT:
            printf("LT\n");
            break;

        case NODE_GT:
            printf("GT\n");
            break;

        case NODE_LE:
            printf("LE\n");
            break;

        case NODE_GE:
            printf("GE\n");
            break;

        case NODE_NE:
            printf("NE\n");
            break;

        case NODE_EQ:
            printf("EQ\n");
            break;

        case NODE_IF:
            printf("IF\n");
            break;

        case NODE_WHILE:
            printf("WHILE\n");
            break;

        case NODE_REPEAT:
            printf("REPEAT\n");
            break;

        case NODE_DOWHILE:
            printf("DOWHILE\n");
            break;

        case NODE_BREAK:
            printf("BREAK\n");
            break;

        case NODE_CONTINUE:
            printf("CONTINUE\n");
            break;

        case NODE_STRCONST:
            printf("STRCONST(%s)\n", root->varname);
            break;
        case NODE_ARRAY:
            printf("ARRAY(%s) ", root->varname);
            if(root->Gentry != NULL)
            {
                printf(" [type=");

                if (root->Gentry->type == TYPE_INT)
                    printf("INT");
                else if (root->Gentry->type == TYPE_STR)
                    printf("STR");

                printf(", binding=%d]\n",
                       root->Gentry->binding);
            }
            break;
        case NODE_2DARRAY:
            printf("2DARRAY(%s) ", root->varname);
            if(root->Gentry != NULL)
            {
                printf(" [type=");
                if (root->Gentry->type == TYPE_INT)
                    printf("INT");
                else if (root->Gentry->type == TYPE_STR)
                    printf("STR");

                printf(", binding=%d]\n",
                       root->Gentry->binding);
            }
            break;

        default:
            printf("UNKNOWN\n");
    }

    printTree(root->left, level + 1);
    printTree(root->middle, level + 1);
    printTree(root->right, level + 1);
}

/* Returns a register containing the address of a 1D or 2D array element. */
static int generateArrayAddress(tnode *arraynode, FILE *target_file)
{
    int addressreg;
    int basereg;

    if (arraynode->nodetype == NODE_ARRAY)
    {
        addressreg = codeGen(arraynode->left, target_file);
    }
    else if (arraynode->nodetype == NODE_2DARRAY)
    {
        int columnreg = codeGen(arraynode->middle, target_file);
        int columnsreg;

        addressreg = codeGen(arraynode->left, target_file);
        columnsreg = getReg();
        fprintf(target_file, "MOV R%d, %d\n", columnsreg, arraynode->Gentry->columns);
        fprintf(target_file, "MUL R%d, R%d\n", addressreg, columnsreg);
        fprintf(target_file, "ADD R%d, R%d\n", addressreg, columnreg);
        freeReg(columnsreg);
        freeReg(columnreg);
    }
    else
    {
        printf("Error: expected an array element\n");
        exit(1);
    }

    basereg = getReg();
    fprintf(target_file, "MOV R%d, %d\n", basereg, arraynode->Gentry->binding);
    fprintf(target_file, "ADD R%d, R%d\n", addressreg, basereg);
    freeReg(basereg);
    return addressreg;
}

int codeGen(tnode *t, FILE *target_file)
{
    int i, j;
    int reg;
    int address;
    int label1, label2, label3;

    if (t == NULL)
        return -1;

    if (t->nodetype == NODE_NUM)
    {
        reg = getReg();
        fprintf(target_file, "MOV R%d, %d\n", reg, t->val);
        return reg;
    }

    if (t->nodetype ==NODE_STRCONST)
    {
        reg = getReg();
        fprintf(target_file, "MOV R%d, %s\n",reg,t->varname);
        return reg;
    }
    
    if (t->nodetype == NODE_ID)
    {
        reg = getReg();
        address = getAddress(t->Gentry);
        fprintf(target_file, "MOV R%d, [%d]\n", reg, address);
        return reg;
    }

    if(t->nodetype == NODE_ARRAY || t->nodetype == NODE_2DARRAY)
    {
        int indexreg = generateArrayAddress(t, target_file);
        int valreg = getReg();
        fprintf(target_file, "MOV R%d, [R%d]\n",valreg,indexreg);
        freeReg(indexreg);
        return valreg;

    }

    if (t->nodetype == NODE_PLUS)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "ADD R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_MINUS)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "SUB R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_MUL)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "MUL R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_DIV)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "DIV R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_LT)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "LT R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_GT)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "GT R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_LE)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "LE R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_GE)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "GE R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_NE)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "NE R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_EQ)
    {
        i = codeGen(t->left, target_file);
        j = codeGen(t->right, target_file);

        fprintf(target_file, "EQ R%d, R%d\n", i, j);
        freeReg(j);

        return i;
    }

    if (t->nodetype == NODE_ASSIGN)
    {
        i = codeGen(t->right, target_file);
        if(t->left->nodetype == NODE_ID)
        {
            address = getAddress(t->left->Gentry);
            fprintf(target_file, "MOV [%d], R%d\n", address, i);
        }
        else if(t->left->nodetype == NODE_ARRAY || t->left->nodetype == NODE_2DARRAY)
        {
            int idxreg = generateArrayAddress(t->left, target_file);
            fprintf(target_file, "MOV [R%d], R%d\n",idxreg,i);
            freeReg(idxreg);
        }

        freeReg(i);

        return -1;
    }

    if (t->nodetype == NODE_READ)
    {
        int addressreg;

        if (t->left->nodetype == NODE_ID)
        {
            addressreg = getReg();
            address = getAddress(t->left->Gentry);
            fprintf(target_file, "MOV R%d, %d\n", addressreg, address);
        }
        else if (t->left->nodetype == NODE_ARRAY || t->left->nodetype == NODE_2DARRAY)
        {
            addressreg = generateArrayAddress(t->left, target_file);
        }
        else
        {
            printf("Error: read requires a variable or array element\n");
            exit(1);
        }

        reg = getReg();

        fprintf(target_file, "MOV R%d, \"Read\"\n", reg);
        fprintf(target_file, "PUSH R%d\n", reg);
        fprintf(target_file, "MOV R%d, -1\n", reg);
        fprintf(target_file, "PUSH R%d\n", reg);
        fprintf(target_file, "PUSH R%d\n", addressreg);
        fprintf(target_file, "PUSH R%d\n", reg);
        fprintf(target_file, "PUSH R%d\n", reg);
        fprintf(target_file, "CALL 0\n");

        fprintf(target_file, "POP R%d\n", reg);
        fprintf(target_file, "POP R%d\n", reg);
        fprintf(target_file, "POP R%d\n", reg);
        fprintf(target_file, "POP R%d\n", reg);
        fprintf(target_file, "POP R%d\n", reg);

        freeReg(reg);
        freeReg(addressreg);

        return -1;
    }

    if (t->nodetype == NODE_WRITE)
    {
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

        breakLabel[loopDepth] = label2;
        continueLabel[loopDepth] = label1;
        loopDepth++;

        fprintf(target_file, "L%d:\n", label1);

        i = codeGen(t->left, target_file);

        fprintf(target_file, "JZ R%d, L%d\n", i, label2);
        freeReg(i);

        codeGen(t->middle, target_file);

        fprintf(target_file, "JMP L%d\n", label1);
        fprintf(target_file, "L%d:\n", label2);

        loopDepth--;

        return -1;
    }

    if (t->nodetype == NODE_REPEAT)
    {
        label1 = GetLabel();
        label2 = GetLabel();
        label3 = GetLabel();

        breakLabel[loopDepth] = label3;
        continueLabel[loopDepth] = label2;
        loopDepth++;

        fprintf(target_file, "L%d:\n", label1);

        codeGen(t->middle, target_file);

        fprintf(target_file, "L%d:\n", label2);

        i = codeGen(t->left, target_file);

        fprintf(target_file, "JZ R%d, L%d\n", i, label1);
        freeReg(i);

        fprintf(target_file, "L%d:\n", label3);

        loopDepth--;

        return -1;
    }

    if (t->nodetype == NODE_DOWHILE)
    {
        label1 = GetLabel();
        label2 = GetLabel();
        label3 = GetLabel();

        breakLabel[loopDepth] = label3;
        continueLabel[loopDepth] = label2;
        loopDepth++;

        fprintf(target_file, "L%d:\n", label1);

        codeGen(t->middle, target_file);

        fprintf(target_file, "L%d:\n", label2);

        i = codeGen(t->left, target_file);

        fprintf(target_file, "JNZ R%d, L%d\n", i, label1);
        freeReg(i);

        fprintf(target_file, "L%d:\n", label3);

        loopDepth--;

        return -1;
    }

    if (t->nodetype == NODE_BREAK)
    {
        if (loopDepth > 0)
        {
            fprintf(target_file,
                    "JMP L%d\n",
                    breakLabel[loopDepth - 1]);
        }

        return -1;
    }

    if (t->nodetype == NODE_CONTINUE)
    {
        if (loopDepth > 0)
        {
            fprintf(target_file,
                    "JMP L%d\n",
                    continueLabel[loopDepth - 1]);
        }

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

void freeGsymbol()
{
    gsymbol *temp;
    gsymbol *next;

    temp = Ghead;

    while(temp != NULL)
    {
        next = temp->next;

        free(temp->name);
        free(temp);

        temp = next;
    }

    Ghead = NULL;
}
