#include "task2.h"

Gsymbol *Ghead = NULL;
Lsymbol *Lhead = NULL;
static int nextBinding = 4096;
static int nextLabel = 0;
static int nextLocalBinding = 1;

static void fail(const char *message, const char *name)
{
    fprintf(stderr, "Error: %s%s%s\n", message, name ? " " : "", name ? name : "");
    exit(EXIT_FAILURE);
}

void semanticError(const char *message, const char *name)
{
    fail(message, name);
}

static const char *typeName(int type)
{
    if (type == TYPE_INT) return "INT";
    if (type == TYPE_STR) return "STR";
    if (type == TYPE_BOOL) return "BOOL";
    return "NONE";
}

Gsymbol *lookupGlobal(const char *name)
{
    Gsymbol *entry;
    for (entry = Ghead; entry; entry = entry->next)
        if (strcmp(entry->name, name) == 0) return entry;
    return NULL;
}

static void rejectGlobalDuplicate(const char *name)
{
    if (lookupGlobal(name)) fail("duplicate global declaration:", name);
}

Paramstruct *makeParam(const char *name, int type)
{
    Paramstruct *param = calloc(1, sizeof(*param));
    if (!param) { perror("calloc"); exit(EXIT_FAILURE); }
    param->name = strdup(name);
    param->type = type;
    return param;
}

Paramstruct *appendParam(Paramstruct *head, Paramstruct *param)
{
    Paramstruct *tail;
    if (!head) return param;
    for (tail = head; tail->next; tail = tail->next) {}
    tail->next = param;
    return head;
}

void freeParams(Paramstruct *params)
{
    while (params) {
        Paramstruct *next = params->next;
        free(params->name);
        free(params);
        params = next;
    }
}

void installGlobalVariable(const char *name, int type, int size, int rows,
                           int columns, int dimension)
{
    Gsymbol *entry;
    rejectGlobalDuplicate(name);
    entry = calloc(1, sizeof(*entry));
    if (!entry) { perror("calloc"); exit(EXIT_FAILURE); }
    entry->name = strdup(name);
    entry->type = type;
    entry->size = size;
    entry->rows = rows;
    entry->columns = columns;
    entry->dimension = dimension;
    entry->binding = nextBinding;
    nextBinding += size;
    entry->flabel = -1;
    entry->next = Ghead;
    Ghead = entry;
}

void installGlobal(const char *name, int type, Paramstruct *params)
{
    Gsymbol *entry;
    Paramstruct *param;
    Paramstruct *other;
    rejectGlobalDuplicate(name);
    for (param = params; param; param = param->next)
        for (other = param->next; other; other = other->next)
            if (strcmp(param->name, other->name) == 0)
                fail("duplicate formal parameter:", param->name);
    entry = calloc(1, sizeof(*entry));
    if (!entry) { perror("calloc"); exit(EXIT_FAILURE); }
    entry->name = strdup(name);
    entry->type = type;
    entry->paramlist = params;
    entry->binding = -1;
    entry->flabel = nextLabel++;
    entry->next = Ghead;
    Ghead = entry;
}

static int paramCount(Paramstruct *params)
{
    int count = 0;
    while (params) { count++; params = params->next; }
    return count;
}

void checkSignature(Gsymbol *function, int type, Paramstruct *params)
{
    Paramstruct *expected;
    Paramstruct *actual;
    if (!function || function->flabel < 0) fail("function has no declaration:", "");
    if (function->type != type) fail("return type mismatch for function:", function->name);
    if (paramCount(function->paramlist) != paramCount(params))
        fail("parameter count mismatch for function:", function->name);
    expected = function->paramlist;
    actual = params;
    while (expected) {
        if (expected->type != actual->type || strcmp(expected->name, actual->name) != 0)
            fail("parameter name or type mismatch for function:", function->name);
        expected = expected->next;
        actual = actual->next;
    }
}

Lsymbol *lookupLocal(const char *name)
{
    Lsymbol *entry;
    for (entry = Lhead; entry; entry = entry->next)
        if (strcmp(entry->name, name) == 0) return entry;
    return NULL;
}

int lookupVisible(const char *name, Gsymbol **global, Lsymbol **local)
{
    *local = lookupLocal(name);
    *global = lookupGlobal(name);
    return *local != NULL || *global != NULL;
}

void beginFunctionScope(Paramstruct *params)
{
    Paramstruct *param;
    int binding = -3;
    Lhead = NULL;
    nextLocalBinding = 1;
    for (param = params; param; param = param->next, binding--)
        installLocal(param->name, param->type), Lhead->binding = binding;
    nextLocalBinding = 1;
}

void installLocal(const char *name, int type)
{
    Lsymbol *entry;
    if (lookupLocal(name)) fail("duplicate local declaration:", name);
    entry = calloc(1, sizeof(*entry));
    if (!entry) { perror("calloc"); exit(EXIT_FAILURE); }
    entry->name = strdup(name);
    entry->type = type;
    entry->binding = nextLocalBinding++;
    entry->next = Lhead;
    Lhead = entry;
}

void endFunctionScope(void)
{
    freeLocalTable(Lhead);
    Lhead = NULL;
}

void freeLocalTable(Lsymbol *locals)
{
    Lsymbol *entry = locals;
    while (entry) { Lsymbol *next = entry->next; free(entry->name); free(entry); entry = next; }
}

tnode *makeNode(int nodetype, int type, int val, const char *name,
                tnode *left, tnode *middle, tnode *right)
{
    tnode *node = calloc(1, sizeof(*node));
    if (!node) { perror("calloc"); exit(EXIT_FAILURE); }
    node->nodetype = nodetype; node->type = type; node->val = val;
    node->left = left; node->middle = middle; node->right = right;
    if (name) node->varname = strdup(name);
    return node;
}

tnode *makeIdentifier(const char *name)
{
    Gsymbol *global;
    Lsymbol *local;
    if (!lookupVisible(name, &global, &local))
        fail("undeclared identifier:", name);
    {
        tnode *node = makeNode(NODE_ID, local ? local->type : global->type, 0, name, NULL, NULL, NULL);
        node->gentry = global;
        node->lentry = local;
        return node;
    }
}

tnode *makeFunctionCall(Gsymbol *function, tnode *args)
{
    Paramstruct *param = function ? function->paramlist : NULL;
    tnode *arg = args;
    while (param && arg) {
        if (param->type != arg->type) fail("argument type mismatch in call to:", function->name);
        param = param->next; arg = arg->next;
    }
    if (param || arg) fail("argument count mismatch in call to:", function->name);
    return makeNode(NODE_FUNCALL, function->type, 0, function->name, args, NULL, NULL);
}

static void printParams(Paramstruct *param)
{
    while (param) { printf("%s %s", typeName(param->type), param->name); if (param->next) printf(", "); param = param->next; }
}

void printGlobals(void)
{
    Gsymbol *entry;
    printf("\nGlobal Symbol Table\n");
    printf("%-16s %-10s %-8s %-6s %-8s %-7s %s\n",
           "Name", "Kind", "Type", "Size", "Binding", "FLabel", "Parameters");
    printf("%-16s %-10s %-8s %-6s %-8s %-7s %s\n",
           "----------------", "----------", "--------", "------",
           "--------", "-------", "----------");
    for (entry = Ghead; entry; entry = entry->next) {
        if (entry->flabel >= 0) {
            printf("%-16s %-10s %-8s %-6s %-8s F%-6d ",
                   entry->name, "FUNCTION", typeName(entry->type), "-", "-", entry->flabel);
            printParams(entry->paramlist);
            printf("\n");
        } else {
            printf("%-16s %-10s %-8s %-6d %-8d %-7s %s\n",
                   entry->name, "VARIABLE", typeName(entry->type), entry->size,
                   entry->binding, "-", "-");
        }
    }
}

void printLocals(void)
{
    Lsymbol *entry;
    printf("%-16s %-8s %s\n", "Name", "Type", "Binding");
    printf("%-16s %-8s %s\n", "----------------", "--------", "-------");
    for (entry = Lhead; entry; entry = entry->next)
        printf("%-16s %-8s BP%+d\n", entry->name, typeName(entry->type), entry->binding);
}

static const char *nodeName(int nodetype)
{
    switch (nodetype) {
        case NODE_NUM: return "NUM";
        case NODE_STRCONST: return "STRCONST";
        case NODE_ID: return "ID";
        case NODE_PLUS: return "PLUS";
        case NODE_MINUS: return "MINUS";
        case NODE_MUL: return "MUL";
        case NODE_DIV: return "DIV";
        case NODE_MOD: return "MOD";
        case NODE_LT: return "LT";
        case NODE_GT: return "GT";
        case NODE_LE: return "LE";
        case NODE_GE: return "GE";
        case NODE_NE: return "NE";
        case NODE_EQ: return "EQ";
        case NODE_ASSIGN: return "ASSIGN";
        case NODE_READ: return "READ";
        case NODE_WRITE: return "WRITE";
        case NODE_IF: return "IF";
        case NODE_CONNECTOR: return "CONNECTOR";
        case NODE_RETURN: return "RETURN";
        case NODE_FUNCALL: return "FUNCALL";
        default: return "UNKNOWN";
    }
}

void printAst(tnode *tree, int level)
{
    int i;
    if (!tree) return;
    for (i = 0; i < level; i++) printf("    ");
    printf("%s", nodeName(tree->nodetype));
    if (tree->nodetype == NODE_NUM)
        printf("(%d)", tree->val);
    else if (tree->varname)
        printf("(%s)", tree->varname);
    if (tree->nodetype == NODE_ID) {
        if (tree->lentry)
            printf(" [local: type=%s, BP%+d]", typeName(tree->lentry->type), tree->lentry->binding);
        else if (tree->gentry)
            printf(" [global: type=%s, binding=%d]", typeName(tree->gentry->type), tree->gentry->binding);
    } else if (tree->nodetype == NODE_FUNCALL && tree->gentry) {
        printf(" [return=%s, label=F%d]", typeName(tree->gentry->type), tree->gentry->flabel);
    }
    printf("\n");
    printAst(tree->left, level + 1); printAst(tree->middle, level + 1); printAst(tree->right, level + 1);
    printAst(tree->next, level + 1);
}

void freeAst(tnode *tree)
{
    if (!tree) return;
    freeAst(tree->left); freeAst(tree->middle); freeAst(tree->right);
    freeAst(tree->next);
    free(tree->varname); free(tree);
}

void freeAll(void)
{
    Gsymbol *entry = Ghead;
    while (entry) { Gsymbol *next = entry->next; Paramstruct *p = entry->paramlist; while (p) { Paramstruct *n = p->next; free(p->name); free(p); p = n; } free(entry->name); free(entry); entry = next; }
    Ghead = NULL;
    Lhead = NULL;
}
