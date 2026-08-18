struct tnode* makeLeafNode(int n) 
{
    struct tnode *newNode = (struct tnode*)malloc(sizeof(struct tnode));
    newNode->val = n;
    newNode->op = NULL;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

struct tnode* makeOperatorNode(char op, struct tnode* left,struct tnode* right)
{
    struct tnode* newnode = (struct tnode*)malloc(sizeof(struct tnode));
    newnode->op = malloc(sizeof(char));
    *(newnode->op) = op;
    newnode->left = left;
    newnode->right = right;
    return newnode;
}

int evaluate(struct tnode* t)
{
    if(t->op == NULL)
    {
        return t->val;
    }
    else{
    switch(*(t->op))
        {
            case '+' : return evaluate(t->left) + evaluate(t->right);
                       break;
            case '-' : return evaluate(t->left) - evaluate(t->right);
                       break;
            case '*' : return evaluate(t->left) * evaluate(t->right);
                       break;
            case '/' : return evaluate(t->left) / evaluate(t->right);
                       break;
        }
    }
}

void prefix(struct tnode* root)
{
    if(root==NULL)
    {
        return;
    }
    else
    {
        if(root->op!=NULL)
        {
            printf("%c",*(root->op));
        }
        else
        {
            printf("%d",root->val);
        }
        prefix(root->left);
        prefix(root->right);
    }

}

void postfix(struct tnode* root)
{
    if(root==NULL)
    {
        return;
    }
    else
    {
        postfix(root->left);
        postfix(root->right);
        if(root->op!=NULL)
        {
            printf("%c",*(root->op));
        }
        else
        {
            printf("%d",root->val);
        }
    }

}