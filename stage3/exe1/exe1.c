#include "exe1.h"
int memory[26];

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



int evaluate(tnode *root)
{
    int leftValue, rightValue;
    int index;

    if (root == NULL)
        return 0;

    switch (root->nodetype)
    {
        case NODE_NUM:
            return root->val;

        case NODE_ID:
            index = root->varname[0] - 'a';
            return memory[index];

        case NODE_PLUS:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue + rightValue;

        case NODE_MINUS:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue - rightValue;

        case NODE_MUL:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue * rightValue;

        case NODE_DIV:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);

            if (rightValue == 0)
            {
                printf("Error: Division by zero\n");
                exit(1);
            }

            return leftValue / rightValue;

        case NODE_LT:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue < rightValue;

        case NODE_GT:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue > rightValue;

        case NODE_LE:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue <= rightValue;

        case NODE_GE:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue >= rightValue;

        case NODE_NE:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue != rightValue;

        case NODE_EQ:
            leftValue = evaluate(root->left);
            rightValue = evaluate(root->right);
            return leftValue == rightValue;

        case NODE_ASSIGN:
            index = root->left->varname[0] - 'a';

            memory[index] = evaluate(root->right);

            return memory[index];

        case NODE_READ:
            index = root->left->varname[0] - 'a';

            scanf("%d", &memory[index]);

            return memory[index];

        case NODE_WRITE:
            leftValue = evaluate(root->left);

            printf("%d\n", leftValue);

            return leftValue;

        case NODE_CONNECTOR:
            evaluate(root->left);
            evaluate(root->right);

            return 0;

        case NODE_PROGRAM:
            evaluate(root->left);

            return 0;

        case NODE_IF:
            leftValue = evaluate(root->left);

            if (leftValue)
                evaluate(root->middle);
            else
                evaluate(root->right);

            return 0;

        case NODE_WHILE:
            while (evaluate(root->left))
                evaluate(root->middle);

            return 0;

        default:
            printf("Unknown node type\n");
            exit(1);
    }
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