typedef struct tnode{
    int val;
    char* op;
    struct tnode* left, *right;
} tnode;

struct tnode* makeLeafNode(int n);

struct tnode* makeOperatorNode(char  op,struct tnode* left,struct tnode* right);

int evaluate(struct tnode* t);