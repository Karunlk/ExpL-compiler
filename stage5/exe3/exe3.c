#include "exe3.h"

Gsymbol *Ghead = NULL;
Lsymbol *Lhead = NULL;
FunctionAst *functions = NULL;
TupleDef *Tuples = NULL;
static int nextBinding = 4096;
static int nextLabel = 0;
static int nextLocalBinding = 1;
static int registers[20];
static FunctionAst *codeFunction;
static int codeIsMain;

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
    if (type == TYPE_INT_PTR) return "INT_PTR";
    if (type == TYPE_STR_PTR) return "STR_PTR";
    if (type == TYPE_TUPLE) return "TUPLE";
    if (type == TYPE_TUPLE_PTR) return "TUPLE_PTR";
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
    param->tuple = NULL;
    return param;
}

static Paramstruct *makeTupleParam(const char *name, int type, TupleDef *tuple)
{
    Paramstruct *param = makeParam(name, type);
    param->tuple = tuple;
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

TupleDef *installTuple(const char *name, TupleField *fields)
{
    TupleDef *tuple = calloc(1, sizeof(*tuple));
    TupleField *field;
    if (!tuple) { perror("calloc"); exit(EXIT_FAILURE); }
    if (Tuples) { TupleDef *tail = Tuples; while (tail->next) tail = tail->next; tail->next = tuple; }
    else Tuples = tuple;
    tuple->name = strdup(name);
    tuple->fields = fields;
    for (field = fields; field; field = field->next) field->offset = tuple->size++;
    return tuple;
}

TupleDef *lookupTuple(const char *name)
{
    TupleDef *tuple;
    for (tuple = Tuples; tuple; tuple = tuple->next)
        if (strcmp(tuple->name, name) == 0) return tuple;
    return NULL;
}

TupleField *appendTupleField(TupleField *head, TupleField *field)
{
    TupleField *tail;
    if (!head) return field;
    for (tail = head; tail->next; tail = tail->next) {}
    tail->next = field;
    return head;
}

TupleField *findField(TupleDef *tuple, const char *name)
{
    TupleField *field;
    for (field = tuple ? tuple->fields : NULL; field; field = field->next)
        if (strcmp(field->name, name) == 0) return field;
    return NULL;
}

void installTupleVariable(const char *name, TupleDef *tuple)
{
    Gsymbol *entry;
    rejectGlobalDuplicate(name);
    entry = calloc(1, sizeof(*entry));
    entry->name = strdup(name); entry->type = TYPE_TUPLE; entry->tuple = tuple;
    entry->size = tuple->size; entry->binding = nextBinding; nextBinding += tuple->size;
    entry->flabel = -1; entry->next = Ghead; Ghead = entry;
}

void installTuplePointerVariable(const char *name, TupleDef *tuple)
{
    Gsymbol *entry;
    rejectGlobalDuplicate(name);
    entry = calloc(1, sizeof(*entry));
    entry->name = strdup(name); entry->type = TYPE_TUPLE_PTR; entry->tuple = tuple;
    entry->size = 1; entry->binding = nextBinding++;
    entry->flabel = -1; entry->next = Ghead; Ghead = entry;
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

void installGlobalTupleFunction(const char *name, int type, TupleDef *tuple, Paramstruct *params)
{
    installGlobal(name, type, params);
    lookupGlobal(name)->tuple = tuple;
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
        if (expected->type != actual->type || expected->tuple != actual->tuple || strcmp(expected->name, actual->name) != 0)
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
        installLocalParameter(param->name, param->type, param->tuple, binding);
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

void installLocalParameter(const char *name, int type, TupleDef *tuple, int binding)
{
    Lsymbol *entry;
    if (lookupLocal(name)) fail("duplicate local declaration:", name);
    entry = calloc(1, sizeof(*entry));
    if (!entry) { perror("calloc"); exit(EXIT_FAILURE); }
    entry->name = strdup(name); entry->type = type; entry->tuple = tuple;
    entry->binding = binding; entry->next = Lhead; Lhead = entry;
}

void installLocalTuple(const char *name, TupleDef *tuple)
{
    Lsymbol *entry;
    if (lookupLocal(name)) fail("duplicate local declaration:", name);
    entry = calloc(1, sizeof(*entry)); entry->name = strdup(name);
    entry->type = TYPE_TUPLE; entry->tuple = tuple; entry->binding = nextLocalBinding++;
    Lhead = entry;
}

void installLocalTuplePointer(const char *name, TupleDef *tuple)
{
    Lsymbol *entry;
    if (lookupLocal(name)) fail("duplicate local declaration:", name);
    entry = calloc(1, sizeof(*entry)); entry->name = strdup(name);
    entry->type = TYPE_TUPLE_PTR; entry->tuple = tuple; entry->binding = nextLocalBinding++;
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
        if (param->type != arg->type ||
            ((param->type == TYPE_TUPLE || param->type == TYPE_TUPLE_PTR) &&
             param->tuple != (arg->lentry ? arg->lentry->tuple : arg->gentry ? arg->gentry->tuple : NULL)))
            fail("argument type mismatch in call to:", function->name);
        param = param->next; arg = arg->next;
    }
    if (param || arg) fail("argument count mismatch in call to:", function->name);
    {
        tnode *call = makeNode(NODE_FUNCALL, function->type, 0, function->name, args, NULL, NULL);
        call->gentry = function;
        return call;
    }
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

static int codeRegister(void)
{
    int i;
    for (i = 0; i < 20; i++) if (!registers[i]) { registers[i] = 1; return i; }
    fail("register exhaustion", NULL);
    return -1;
}

static void releaseRegister(int reg)
{
    if (reg >= 0 && reg < 20) registers[reg] = 0;
}

static int generateNode(tnode *node, FILE *out);

static int variableAddress(tnode *node, FILE *out)
{
    int address;
    if (node->lentry) {
        int reg = codeRegister();
        fprintf(out, "MOV R%d, BP\nADD R%d, %d\n", reg, reg, node->lentry->binding);
        return reg;
    }
    address = node->gentry->binding;
    return address;
}

static int arrayAddress(tnode *node, FILE *out)
{
    int index = generateNode(node->left, out);
    int base = codeRegister();
    if (node->middle) {
        int column = generateNode(node->middle, out);
        fprintf(out, "MUL R%d, %d\nADD R%d, R%d\n", index, node->gentry->columns, index, column);
        releaseRegister(column);
    }
    fprintf(out, "MOV R%d, %d\nADD R%d, R%d\n", base,
            node->gentry->binding, base, index);
    releaseRegister(index);
    return base;
}

static int fieldAddress(tnode *node, FILE *out)
{
    if (node->lentry) {
        int reg = codeRegister();
        fprintf(out, "MOV R%d, BP\nADD R%d, %d\n", reg, reg, node->lentry->binding);
        if (node->lentry->binding < 0) {
            fprintf(out, "MOV R%d, [R%d]\n", reg, reg);
            fprintf(out, "ADD R%d, %d\n", reg, node->val);
        } else {
            fprintf(out, "ADD R%d, %d\n", reg, node->val);
        }
        return reg;
    }
    if (node->gentry->type == TYPE_TUPLE_PTR) {
        int reg = codeRegister();
        fprintf(out, "MOV R%d, [%d]\nADD R%d, %d\n", reg, node->gentry->binding, reg, node->val);
        return reg;
    }
    return node->gentry->binding + node->val;
}

static int tupleAddress(tnode *node, FILE *out)
{
    int reg = codeRegister();
    if (node->lentry) {
        fprintf(out, "MOV R%d, BP\nADD R%d, %d\n", reg, reg, node->lentry->binding);
        if (node->lentry->binding < 0)
            fprintf(out, "MOV R%d, [R%d]\n", reg, reg);
    } else
        fprintf(out, "MOV R%d, %d\n", reg, node->gentry->binding);
    return reg;
}

static int generateNode(tnode *node, FILE *out)
{
    int left, right, reg, address;
    if (!node) return -1;
    switch (node->nodetype) {
        case NODE_NUM:
            reg = codeRegister(); fprintf(out, "MOV R%d, %d\n", reg, node->val); return reg;
        case NODE_STRCONST:
            reg = codeRegister(); fprintf(out, "MOV R%d, %s\n", reg, node->varname); return reg;
        case NODE_ID:
            if (node->lentry) {
                reg = variableAddress(node, out);
                fprintf(out, "MOV R%d, [R%d]\n", reg, reg);
                return reg;
            }
            reg = codeRegister(); fprintf(out, "MOV R%d, [%d]\n", reg, node->gentry->binding); return reg;
        case NODE_ADDRESS:
            if (node->lentry) {
                reg = codeRegister();
                fprintf(out, "MOV R%d, BP\nADD R%d, %d\n", reg, reg, node->lentry->binding);
            } else {
                reg = codeRegister();
                fprintf(out, "MOV R%d, %d\n", reg, node->gentry->binding);
            }
            return reg;
        case NODE_DEREF:
            reg = generateNode(node->left, out);
            fprintf(out, "MOV R%d, [R%d]\n", reg, reg);
            return reg;
        case NODE_ARRAY:
            address = arrayAddress(node, out);
            reg = codeRegister(); fprintf(out, "MOV R%d, [R%d]\n", reg, address);
            releaseRegister(address);
            return reg;
        case NODE_FIELD:
            address = fieldAddress(node, out);
            reg = codeRegister();
            if (node->lentry) fprintf(out, "MOV R%d, [R%d]\n", reg, address);
            else fprintf(out, "MOV R%d, [%d]\n", reg, address);
            releaseRegister(address);
            return reg;
        case NODE_PLUS: case NODE_MINUS: case NODE_MUL: case NODE_DIV: case NODE_MOD:
        case NODE_LT: case NODE_GT: case NODE_LE: case NODE_GE: case NODE_NE: case NODE_EQ:
            left = generateNode(node->left, out); right = generateNode(node->right, out);
            fprintf(out, "%s R%d, R%d\n", node->nodetype == NODE_PLUS ? "ADD" :
                    node->nodetype == NODE_MINUS ? "SUB" : node->nodetype == NODE_MUL ? "MUL" :
                    node->nodetype == NODE_DIV ? "DIV" : node->nodetype == NODE_MOD ? "MOD" :
                    node->nodetype == NODE_LT ? "LT" : node->nodetype == NODE_GT ? "GT" :
                    node->nodetype == NODE_LE ? "LE" : node->nodetype == NODE_GE ? "GE" :
                    node->nodetype == NODE_NE ? "NE" : "EQ", left, right);
            releaseRegister(right); return left;
        case NODE_ASSIGN:
            if (node->left->type == TYPE_TUPLE) {
                TupleField *field = node->left->lentry ? node->left->lentry->tuple->fields : node->left->gentry->tuple->fields;
                int sourceBase = node->right->nodetype == NODE_FUNCALL
                    ? generateNode(node->right, out)
                    : tupleAddress(node->right, out);
                int targetBase = tupleAddress(node->left, out);
                for (; field; field = field->next) {
                    int sourceAddress = codeRegister();
                    int targetAddress = codeRegister();
                    int value = codeRegister();
                    fprintf(out, "MOV R%d, R%d\nADD R%d, %d\n", sourceAddress, sourceBase, sourceAddress, field->offset);
                    fprintf(out, "MOV R%d, [R%d]\n", value, sourceAddress);
                    fprintf(out, "MOV R%d, R%d\nADD R%d, %d\nMOV [R%d], R%d\n", targetAddress, targetBase, targetAddress, field->offset, targetAddress, value);
                    releaseRegister(sourceAddress); releaseRegister(targetAddress); releaseRegister(value);
                }
                releaseRegister(sourceBase); releaseRegister(targetBase);
                return -1;
            }
            right = generateNode(node->right, out);
            if (node->left->nodetype == NODE_DEREF) {
                address = generateNode(node->left->left, out);
                fprintf(out, "MOV [R%d], R%d\n", address, right);
                releaseRegister(address);
            } else if (node->left->nodetype == NODE_FIELD) {
                address = fieldAddress(node->left, out);
                if (node->left->lentry) fprintf(out, "MOV [R%d], R%d\n", address, right);
                else fprintf(out, "MOV [%d], R%d\n", address, right);
                releaseRegister(address);
            } else if (node->left->nodetype == NODE_ARRAY) {
                address = arrayAddress(node->left, out);
                fprintf(out, "MOV [R%d], R%d\n", address, right);
                releaseRegister(address);
            } else if (node->left->lentry) {
                address = variableAddress(node->left, out);
                fprintf(out, "MOV [R%d], R%d\n", address, right); releaseRegister(address);
            } else fprintf(out, "MOV [%d], R%d\n", node->left->gentry->binding, right);
            releaseRegister(right); return -1;
        case NODE_CONNECTOR:
            generateNode(node->left, out); generateNode(node->right, out); return -1;
        case NODE_AND:
            left = generateNode(node->left, out);
            right = generateNode(node->right, out);
            fprintf(out, "MUL R%d, R%d\n", left, right);
            releaseRegister(right);
            return left;
        case NODE_WHILE:
            { int start = nextLabel++, end = nextLabel++;
              fprintf(out, "L%d:\n", start);
              left = generateNode(node->left, out);
              fprintf(out, "JZ R%d, L%d\n", left, end);
              releaseRegister(left);
              generateNode(node->middle, out);
              fprintf(out, "JMP L%d\nL%d:\n", start, end);
              return -1; }
        case NODE_IF:
            left = generateNode(node->left, out);
            { int elseLabel = nextLabel++, endLabel = nextLabel++;
              fprintf(out, "JZ R%d, L%d\n", left, elseLabel); releaseRegister(left);
              generateNode(node->middle, out); fprintf(out, "JMP L%d\nL%d:\n", endLabel, elseLabel);
              generateNode(node->right, out); fprintf(out, "L%d:\n", endLabel); }
            return -1;
        case NODE_READ:
            reg = codeRegister();
            if (node->left->nodetype == NODE_ARRAY) { address = arrayAddress(node->left, out); fprintf(out, "MOV R%d, \"Read\"\nPUSH R%d\nMOV R%d, -1\nPUSH R%d\nPUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", reg, reg, reg, reg, address, reg, reg, reg, reg, reg, reg, reg); releaseRegister(address); }
            else if (node->left->lentry) { address = variableAddress(node->left, out); fprintf(out, "MOV R%d, \"Read\"\nPUSH R%d\nMOV R%d, -1\nPUSH R%d\nPUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", reg, reg, reg, reg, address, reg, reg, reg, reg, reg, reg, reg); releaseRegister(address); }
            else { fprintf(out, "MOV R%d, \"Read\"\nPUSH R%d\nMOV R%d, -1\nPUSH R%d\nMOV R%d, %d\nPUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", reg, reg, reg, reg, reg, node->left->gentry->binding, reg, reg, reg, reg, reg, reg, reg, reg); }
            releaseRegister(reg); return -1;
        case NODE_WRITE:
            left = generateNode(node->left, out); reg = codeRegister();
            fprintf(out, "MOV R%d, \"Write\"\nPUSH R%d\nMOV R%d, -2\nPUSH R%d\nPUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", reg, reg, reg, reg, left, reg, reg, reg, reg, reg, reg, reg); releaseRegister(left); releaseRegister(reg); return -1;
        case NODE_RETURN:
            if (codeIsMain) { generateNode(node->left, out); fprintf(out, "INT 10\n"); return -1; }
            if (node->left->type == TYPE_TUPLE || node->left->type == TYPE_TUPLE_PTR)
                left = node->left->nodetype == NODE_ID ? tupleAddress(node->left, out) : generateNode(node->left, out);
            else
                left = generateNode(node->left, out);
            reg = codeRegister();
            fprintf(out, "MOV R%d, BP\nSUB R%d, 2\nMOV [R%d], R%d\n", reg, reg, reg, left);
            releaseRegister(left);
            releaseRegister(reg);
            if (codeFunction->locals) { int count = 0; Lsymbol *local; for (local = codeFunction->locals; local; local = local->next) if (local->binding > 0) count++; while (count--) fprintf(out, "POP R0\n"); }
            fprintf(out, "POP BP\nRET\n"); return -1;
        case NODE_FUNCALL:
            { Gsymbol *function = node->gentry; tnode *arg; int saved[20], savedCount = 0, i;
              for (i = 0; i < 20; i++) if (registers[i]) { fprintf(out, "PUSH R%d\n", i); saved[savedCount++] = i; }
              int count = 0; for (arg = node->left; arg; arg = arg->next) count++;
              tnode **args = malloc(sizeof(*args) * count); arg = node->left; for (i = 0; i < count; i++, arg = arg->next) args[i] = arg;
              for (i = count - 1; i >= 0; i--) {
                  left = (args[i]->type == TYPE_TUPLE || args[i]->type == TYPE_TUPLE_PTR)
                      ? tupleAddress(args[i], out) : generateNode(args[i], out);
                  fprintf(out, "PUSH R%d\n", left); releaseRegister(left);
              }
              fprintf(out, "PUSH R0\nCALL F%d\n", function->flabel);
              reg = codeRegister();
              fprintf(out, "POP R%d\n", reg);
              if (count > 0) {
                  int discard = codeRegister();
                  for (i = 0; i < count; i++) fprintf(out, "POP R%d\n", discard);
                  releaseRegister(discard);
              }
              for (i = savedCount - 1; i >= 0; i--)
                  fprintf(out, "POP R%d\n", saved[i]);
              free(args); return reg; }
        default: fail("unsupported AST node during code generation", NULL);
    }
    return -1;
}

void generateProgram(const char *filename)
{
    FILE *out = fopen(filename, "w");
    FunctionAst *function;
    if (!out) { perror("output.o"); exit(EXIT_FAILURE); }
    fprintf(out, "0\n2056\n0\n0\n0\n0\n0\n0\nBRKP\nMOV SP, %d\n", nextBinding - 1);
    memset(registers, 0, sizeof(registers));
    for (function = functions; function; function = function->next) {
        if (strcmp(function->name, "main") == 0) {
            codeFunction = function; codeIsMain = 1; fprintf(out, "MOV BP, SP\n");
            { int locals = 0; Lsymbol *local; for (local = function->locals; local; local = local->next) if (local->binding > 0) locals++; while (locals--) fprintf(out, "PUSH R0\n"); }
            generateNode(function->tree, out); fprintf(out, "INT 10\n");
        }
    }
    for (function = functions; function; function = function->next) if (strcmp(function->name, "main") != 0) {
        Gsymbol *entry = lookupGlobal(function->name); Lsymbol *local; int locals = 0;
        codeFunction = function; codeIsMain = 0; fprintf(out, "F%d:\nPUSH BP\nMOV BP, SP\n", entry->flabel);
        for (local = function->locals; local; local = local->next) if (local->binding > 0) locals++;
        while (locals--) fprintf(out, "PUSH R0\n");
        generateNode(function->tree, out);
    }
    fclose(out);
}
