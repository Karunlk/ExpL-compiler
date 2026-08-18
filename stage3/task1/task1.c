#include "task1.h"

tnode* createTree(int val,int type,char *c,int nodetype,tnode *l,tnode *m,tnode *r)
    {
        tnode *temp;

        if (nodetype == NODE_PLUS || nodetype == NODE_MINUS || nodetype == NODE_MUL || nodetype == NODE_DIV)
        {
            if (l->type != TYPE_INT || r->type != TYPE_INT)
            {
                printf("Type mismatch: arithmetic operator requires integer operands\n");
                exit(1);
            }
            type = TYPE_INT;
        }

        else if (nodetype == NODE_LT || nodetype == NODE_GT || nodetype == NODE_LE || nodetype == NODE_GE || nodetype == NODE_NE || nodetype == NODE_EQ)
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

void printTree(tnode *root, int level)
    {
        if (root == NULL)
        return;

        for (int i = 0; i < level; i++)
        printf("    ");

        switch (root->nodetype)
        {
            case NODE_NUM:
            printf("NUM(%d) [INT]\n", root->val);
            break;

            case NODE_ID:
            printf("ID(%s) [INT]\n", root->varname);
            break;

            case NODE_PLUS:
            printf("PLUS [INT]\n");
            break;

            case NODE_MINUS:
            printf("MINUS [INT]\n");
            break;

            case NODE_MUL:
            printf("MUL [INT]\n");
            break;

            case NODE_DIV:
            printf("DIV [INT]\n");
            break;

            case NODE_LT:
            printf("LT [BOOL]\n");
            break;

            case NODE_GT:
            printf("GT [BOOL]\n");
            break;

            case NODE_LE:
            printf("LE [BOOL]\n");
            break;

            case NODE_GE:
            printf("GE [BOOL]\n");
            break;

            case NODE_NE:
            printf("NE [BOOL]\n");
            break;

            case NODE_EQ:
            printf("EQ [BOOL]\n");
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

            case NODE_CONNECTOR:
            printf("CONNECTOR\n");
            break;

            case NODE_PROGRAM:
            printf("PROGRAM\n");
            break;

            case NODE_IF:
            printf("IF\n");
            break;

            case NODE_WHILE:
            printf("WHILE\n");
            break;

            default:
            printf("UNKNOWN\n");
        }

        printTree(root->left, level + 1);
        printTree(root->middle, level + 1);
        printTree(root->right, level + 1);
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