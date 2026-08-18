#include "exe1.h"


int memory[26];


tnode* createTree(int val, int type, char *c,tnode *l, tnode *r)
{
    tnode *temp = (tnode *)malloc(sizeof(tnode));

    if (temp == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    temp->val = val;
    temp->type = type;
    temp->left = l;
    temp->right = r;

    if (c != NULL) {
        temp->varname = strdup(c);
    }
    else {
        temp->varname = NULL;
    }

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

            if (rightValue == 0) {
                printf("Error: Division by zero\n");
                exit(1);
            }

            return leftValue / rightValue;

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
    freeTree(root->right);

    if (root->varname != NULL)
        free(root->varname);

    free(root);
}