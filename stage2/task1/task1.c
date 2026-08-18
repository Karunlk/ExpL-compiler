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

void printTree(tnode *root, int level)
{
    if (root == NULL)
        return;

    for (int i = 0; i < level; i++)
        printf("    ");

    switch (root->nodetype) {

        case NODE_NUM:
            printf("NUM(%d)\n", root->val);
            break;

        case NODE_ID:
            printf("ID(%s)\n", root->varname);
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

        case NODE_CONNECTOR:
            printf("CONNECTOR\n");
            break;

        case NODE_PROGRAM:
            printf("PROGRAM\n");
            break;

        default:
            printf("UNKNOWN\n");
    }

    printTree(root->left, level + 1);
    printTree(root->right, level + 1);
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