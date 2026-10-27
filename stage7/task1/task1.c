#include "task1.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern void yyerror(const char *s);

int reg_index = -1;
int label_index = -1;

int break_labels[20];
int continue_labels[20];
int loop_top = -1;

/* Function currently being parsed (for return type checking) */
Gsymbol* current_function = NULL;
int current_return_type = INTEGER_TYPE;

void semanticError(const char* fmt, const char* name) {
    char msg[200];
    snprintf(msg, sizeof(msg), fmt, name);
    yyerror(msg);
    exit(1);
}

int getLabel() {
    label_index++;
    return label_index;
}

int getReg() {
    if(reg_index < 19) {
        reg_index++;
        return reg_index;
    }
    printf("No registers available\n");
    exit(1);
}

void freeReg() {
    if(reg_index >= 0) {
        reg_index--;
    }
}

void setCurrentFunction(Gsymbol* f, int returnType) {
    current_function = f;
    current_return_type = returnType;
}

int getCurrentReturnType() {
    return current_return_type;
}

int pointerTo(int base_type) {
    if(base_type == INTEGER_TYPE) return INTEGER_POINTER_TYPE;
    if(base_type == STRING_TYPE) return STRING_POINTER_TYPE;
    yyerror("invalid pointer base type");
    exit(1);
}

/* Name equivalence: types must be identical; NULL is compatible with every user-defined type */
static int argCompatible(int lt, int rt) {
    return lt == rt || (isUserType(lt) && rt == NULL_TYPE);
}

static int assignable(int lt, int rt) {
    return argCompatible(lt, rt) || (isUserType(lt) && rt == ALLOC_TYPE);
}

static int comparable(int lt, int rt) {
    if(lt == ALLOC_TYPE || rt == ALLOC_TYPE) return 0;
    return argCompatible(lt, rt) || argCompatible(rt, lt);
}

static int isArrayType(int type) {
    return type == INTEGER_ARRAY_TYPE || type == STRING_ARRAY_TYPE;
}

static int isPointerType(int type) {
    return type == INTEGER_POINTER_TYPE || type == STRING_POINTER_TYPE;
}

tnode* createNode(int val, int type, int nodetype, char* varname, tnode *l, tnode *r, tnode* m) {
    tnode* newNode = (tnode*)malloc(sizeof(tnode));
    newNode->val = val;
    newNode->left = l;
    newNode->right = r;
    newNode->mid = m;
    newNode->type = type;
    newNode->varname = varname != NULL ? strdup(varname) : NULL;
    newNode->nodetype = nodetype;
    newNode->Gentry = NULL;
    newNode->Lentry = NULL;

    switch(nodetype) {
        case PLUS_NODE: case MINUS_NODE: case MUL_NODE: case DIV_NODE: case MODULUS_NODE:
            if(l->type != INTEGER_TYPE || r->type != INTEGER_TYPE) {
                yyerror("type mismatch: arithmetic operands must be integers");
                exit(1);
            }
            newNode->type = INTEGER_TYPE;
            break;

        case LT_NODE: case GT_NODE: case LE_NODE: case GE_NODE: case EQ_NODE: case NE_NODE:
            if(!comparable(l->type, r->type) || isArrayType(l->type) || l->type == BOOLEAN_TYPE) {
                yyerror("type mismatch in comparison");
                exit(1);
            }
            newNode->type = BOOLEAN_TYPE;
            break;

        case AND_NODE: case OR_NODE:
            if(l->type != BOOLEAN_TYPE || r->type != BOOLEAN_TYPE) {
                yyerror("type mismatch: logical operands must be boolean");
                exit(1);
            }
            newNode->type = BOOLEAN_TYPE;
            break;

        case NOT_NODE:
            if(l->type != BOOLEAN_TYPE) {
                yyerror("type mismatch: operand of not must be boolean");
                exit(1);
            }
            newNode->type = BOOLEAN_TYPE;
            break;

        case IF_NODE: case WHILE_NODE: case DO_WHILE_NODE: case REPEAT_UNTIL_NODE:
            if(l->type != BOOLEAN_TYPE) {
                yyerror("type mismatch: condition must be boolean");
                exit(1);
            }
            break;

        case DEREF_NODE:
            if(l->type == INTEGER_POINTER_TYPE) newNode->type = INTEGER_TYPE;
            else if(l->type == STRING_POINTER_TYPE) newNode->type = STRING_TYPE;
            else {
                yyerror("type mismatch: dereferencing a non-pointer");
                exit(1);
            }
            break;

        case ADDRESS_NODE:
            if(l->nodetype != VARIABLE || (l->type != INTEGER_TYPE && l->type != STRING_TYPE)) {
                yyerror("& can only be applied to an int/str variable");
                exit(1);
            }
            newNode->type = pointerTo(l->type);
            break;

        case ASSIGNMENT:
            if(isArrayType(l->type)) {
                yyerror("cannot assign value directly to an array name");
                exit(1);
            }
            if(!assignable(l->type, r->type)) {
                yyerror("type mismatch in assignment");
                exit(1);
            }
            break;

        case WRITE_NODE:
            if(l->type != INTEGER_TYPE && l->type != STRING_TYPE && !isPointerType(l->type)) {
                yyerror("write expects an int or str expression");
                exit(1);
            }
            break;

        case READ_NODE:
            if(l->type != INTEGER_TYPE && l->type != STRING_TYPE) {
                yyerror("read expects an int or str variable");
                exit(1);
            }
            break;

        case FREE_NODE:
            if(!isUserType(l->type)) {
                yyerror("free expects a variable of a user-defined type");
                exit(1);
            }
            break;

        case RETURN_NODE:
            if(!argCompatible(current_return_type, l->type)) {
                yyerror("type mismatch: return type does not match function type");
                exit(1);
            }
            break;
    }

    return newNode;
}

tnode* makeFieldNode(tnode* base, char* field) {
    if(!isUserType(base->type)) semanticError("%s is not a variable of a user-defined type", base->varname);
    Field* f = FLookup(base->type, field);
    /*
     * A class method has the same lexical form as a field until the following
     * '('.  Keep an unresolved member node so the parser can turn it into a
     * method-call AST node.  A real field retains its normal type and offset.
     */
    if(f == NULL)
        return createNode(0, base->type, FIELD_NODE, field, base, NULL, NULL);
    return createNode(f->index, f->type, FIELD_NODE, field, base, NULL, NULL);
}

/* Look up the local symbol table first, then the global symbol table */
tnode* makeVarNode(char* name, tnode* index1, tnode* index2) {
    tnode* node = createNode(0, INTEGER_TYPE, VARIABLE, name, index1, index2, NULL);

    Lsymbol* L = LLookup(name);
    if(L != NULL) {
        if(index1 != NULL) semanticError("%s is not an array", name);
        node->Lentry = L;
        node->type = L->type;
        return node;
    }

    Gsymbol* G = Lookup(name);
    if(G == NULL) semanticError("undeclared variable %s", name);
    if(G->isFunction) semanticError("%s is a function, not a variable", name);
    node->Gentry = G;

    if(isArrayType(G->type)) {
        if(index1 == NULL) {
            node->type = G->type;      // bare array name
        } else {
            if(index1->type != INTEGER_TYPE || (index2 && index2->type != INTEGER_TYPE)) {
                semanticError("array index of %s must be an integer", name);
            }
            node->type = (G->type == INTEGER_ARRAY_TYPE) ? INTEGER_TYPE : STRING_TYPE;
        }
    } else {
        if(index1 != NULL) semanticError("%s is not an array", name);
        node->type = G->type;
    }
    return node;
}

tnode* makeCallNode(char* name, tnode* args) {
    Gsymbol* G = Lookup(name);
    if(G == NULL || !G->isFunction) semanticError("call to undeclared function %s", name);

    /* check number and types of arguments against the declaration */
    Paramstruct* p = G->paramlist;
    tnode* a = args;
    while(p != NULL && a != NULL) {
        if(!argCompatible(p->type, a->left->type)) semanticError("type mismatch in argument to %s", name);
        p = p->next;
        a = a->right;
    }
    if(p != NULL || a != NULL) semanticError("wrong number of arguments in call to %s", name);

    tnode* node = createNode(0, G->type, FUNC_CALL_NODE, name, args, NULL, NULL);
    node->Gentry = G;
    return node;
}

/* ---------------- AST printing ---------------- */

static const char* nodeName(int nt) {
    switch(nt) {
        case STATEMENT: return "SEQ";
        case VARIABLE: return "VAR";
        case ASSIGNMENT: return "ASSIGN";
        case PLUS_NODE: return "+";
        case MINUS_NODE: return "-";
        case MUL_NODE: return "*";
        case DIV_NODE: return "/";
        case MODULUS_NODE: return "%";
        case CONSTANT: return "CONST";
        case READ_NODE: return "READ";
        case WRITE_NODE: return "WRITE";
        case EQ_NODE: return "==";
        case NE_NODE: return "!=";
        case GT_NODE: return ">";
        case LT_NODE: return "<";
        case GE_NODE: return ">=";
        case LE_NODE: return "<=";
        case IF_NODE: return "IF";
        case WHILE_NODE: return "WHILE";
        case DO_WHILE_NODE: return "DO_WHILE";
        case REPEAT_UNTIL_NODE: return "REPEAT_UNTIL";
        case BREAK_NODE: return "BREAK";
        case CONTINUE_NODE: return "CONTINUE";
        case STRING_CONSTANT_NODE: return "STRING";
        case ADDRESS_NODE: return "ADDRESS(&)";
        case DEREF_NODE: return "DEREF(*)";
        case FUNC_CALL_NODE: return "CALL";
        case ARG_NODE: return "ARG";
        case RETURN_NODE: return "RETURN";
        case AND_NODE: return "AND";
        case OR_NODE: return "OR";
        case NOT_NODE: return "NOT";
        case FIELD_NODE: return "FIELD(.)";
        case ALLOC_NODE: return "ALLOC";
        case FREE_NODE: return "FREE";
        case INIT_NODE: return "INITIALIZE";
    }
    return "?";
}

static void indent(int depth) {
    int i;
    for(i = 0; i < depth; i++) printf("|   ");
}

/* Prints t and its subtrees; role names the edge from the parent ("cond", "then", ...) */
static void printNode(tnode* t, int depth, const char* role) {
    if(t == NULL) return;
    indent(depth);
    if(role != NULL) printf("%s: ", role);
    printf("%s", nodeName(t->nodetype));

    switch(t->nodetype) {
        case CONSTANT:
            if(t->type == NULL_TYPE) printf(" null"); else printf(" %d", t->val);
            break;
        case STRING_CONSTANT_NODE:
            printf(" %s", t->varname);
            break;
        case VARIABLE:
            printf(" %s (%s)", t->varname, t->Lentry != NULL ? "local" : "global");
            break;
        case FIELD_NODE:
            printf(" %s (offset %d)", t->varname, t->val);
            break;
        case FUNC_CALL_NODE:
            printf(" %s (F%d)", t->varname, t->Gentry->flabel);
            break;
    }
    switch(t->nodetype) {   /* statements have no type to show */
        case STATEMENT: case ASSIGNMENT: case IF_NODE: case WHILE_NODE: case DO_WHILE_NODE:
        case REPEAT_UNTIL_NODE: case READ_NODE: case WRITE_NODE: case BREAK_NODE:
        case CONTINUE_NODE: case RETURN_NODE:
            printf("\n");
            break;
        default:
            printf("  : %s\n", typeName(t->type));
    }

    switch(t->nodetype) {
        case STATEMENT: {
            /* flatten the left-nested chain of statements */
            tnode* stmts[512];
            int n = 0;
            tnode* c = t;
            while(c != NULL && c->nodetype == STATEMENT && n < 511) {
                stmts[n++] = c->right;
                c = c->left;
            }
            stmts[n++] = c;
            while(n > 0) printNode(stmts[--n], depth + 1, NULL);
            break;
        }
        case IF_NODE:
            printNode(t->left, depth + 1, "cond");
            printNode(t->mid, depth + 1, "then");
            printNode(t->right, depth + 1, "else");
            break;
        case WHILE_NODE: case DO_WHILE_NODE: case REPEAT_UNTIL_NODE:
            printNode(t->left, depth + 1, "cond");
            printNode(t->right, depth + 1, "body");
            break;
        case ASSIGNMENT:
            printNode(t->left, depth + 1, "lhs");
            printNode(t->right, depth + 1, "rhs");
            break;
        case VARIABLE:
            printNode(t->left, depth + 1, "index");
            printNode(t->right, depth + 1, "index2");
            break;
        case FIELD_NODE:
            printNode(t->left, depth + 1, "base");
            break;
        case FUNC_CALL_NODE: {
            tnode* a;
            for(a = t->left; a != NULL; a = a->right) printNode(a->left, depth + 1, "arg");
            break;
        }
        default:
            printNode(t->left, depth + 1, NULL);
            printNode(t->right, depth + 1, NULL);
            printNode(t->mid, depth + 1, NULL);
    }
}

void printAST(char* fname, tnode* body) {
    printf("---- AST of %s ----\n", fname);
    if(body == NULL) printf("(empty body)\n");
    printNode(body, 0, NULL);
    printf("\n");
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- Global symbol table ---------------- */

Gsymbol* head = NULL;
Gsymbol* tail = NULL;
int bind_start = 4096;
int flabel_count = 0;

Gsymbol* Lookup(char* name) {
    Gsymbol* temp = head;
    while(temp != NULL && strcmp(temp->name, name) != 0) {
        temp = temp->next;
    }
    return temp;
}

Gsymbol* get_global_head() {
    return head;
}

static void appendGlobal(Gsymbol* newentry) {
    if(head == NULL) {
        head = newentry;
        tail = head;
    } else {
        tail->next = newentry;
        tail = newentry;
    }
}

int Install(char* name, int type, int size, int* metadata) {
    if(Lookup(name) != NULL) {
        return 1;
    }

    Gsymbol* newentry = (Gsymbol*) malloc(sizeof(Gsymbol));
    newentry->name = strdup(name);
    newentry->size = size;
    newentry->type = type;
    newentry->binding = bind_start;
    bind_start += size;
    newentry->metadata = metadata;
    newentry->paramlist = NULL;
    newentry->flabel = -1;
    newentry->isFunction = 0;
    newentry->defined = 0;
    newentry->next = NULL;

    appendGlobal(newentry);
    return 0;
}

int InstallFunction(char* name, int type, Paramstruct* paramlist) {
    if(Lookup(name) != NULL) {
        return 1;
    }

    Gsymbol* newentry = (Gsymbol*) malloc(sizeof(Gsymbol));
    newentry->name = strdup(name);
    newentry->size = 0;
    newentry->type = type;
    newentry->binding = -1;
    newentry->metadata = NULL;
    newentry->paramlist = paramlist;
    newentry->flabel = flabel_count++;
    newentry->isFunction = 1;
    newentry->defined = 0;
    newentry->next = NULL;

    appendGlobal(newentry);
    return 0;
}

void print_symbol_table() {
    Gsymbol* temp = head;
    printf("---- Global Symbol Table ----\n");
    printf("%-15s %-7s %-7s %-8s %-7s\n", "name", "type", "size", "binding", "flabel");
    while(temp != NULL) {
        if(temp->isFunction) {
            printf("%-15s %-7d %-7d %-8s F%-6d (", temp->name, temp->type, temp->size, "-", temp->flabel);
            Paramstruct* p = temp->paramlist;
            while(p != NULL) {
                printf("%d %s%s", p->type, p->name, p->next ? ", " : "");
                p = p->next;
            }
            printf(")\n");
        } else {
            printf("%-15s %-7d %-7d %-8d %-7s\n", temp->name, temp->type, temp->size, temp->binding, "-");
        }
        temp = temp->next;
    }
}

/* Address of the last word used by global variables (initial SP) */
int get_stack_pointer() {
    return bind_start - 1;
}

/* ---------------- Parameter lists ---------------- */

Paramstruct* makeParam(char* name, int type) {
    Paramstruct* p = (Paramstruct*) malloc(sizeof(Paramstruct));
    p->name = strdup(name);
    p->type = type;
    p->next = NULL;
    return p;
}

Paramstruct* appendParam(Paramstruct* list, Paramstruct* p) {
    if(list == NULL) return p;
    Paramstruct* temp = list;
    while(temp->next != NULL) temp = temp->next;
    temp->next = p;
    return list;
}

int countParams(Paramstruct* list) {
    int n = 0;
    while(list != NULL) {
        n++;
        list = list->next;
    }
    return n;
}

/* ---------------- Local symbol table ---------------- */

Lsymbol* Lhead = NULL;
Lsymbol* Ltail = NULL;
int local_count = 0;

void LReset() {
    Lhead = NULL;
    Ltail = NULL;
    local_count = 0;
}

Lsymbol* LLookup(char* name) {
    Lsymbol* temp = Lhead;
    while(temp != NULL && strcmp(temp->name, name) != 0) {
        temp = temp->next;
    }
    return temp;
}

static int LAppend(char* name, int type, int binding) {
    if(LLookup(name) != NULL) {
        return 1;
    }
    Lsymbol* newentry = (Lsymbol*) malloc(sizeof(Lsymbol));
    newentry->name = strdup(name);
    newentry->type = type;
    newentry->binding = binding;
    newentry->next = NULL;

    if(Lhead == NULL) {
        Lhead = newentry;
        Ltail = newentry;
    } else {
        Ltail->next = newentry;
        Ltail = newentry;
    }
    return 0;
}

/* Parameter i (1-based) is at BP-(i+2): [BP-1] = return address, [BP-2] = return value */
int LInstallParams(Paramstruct* params) {
    int binding = -3;
    while(params != NULL) {
        if(LAppend(params->name, params->type, binding)) {
            return 1;
        }
        binding--;
        params = params->next;
    }
    return 0;
}

/* Local variable i (1-based) is at BP+i */
int LInstall(char* name, int type) {
    if(LAppend(name, type, local_count + 1)) {
        return 1;
    }
    local_count++;
    return 0;
}

int LLocalCount() {
    return local_count;
}

void print_local_table(char* fname) {
    Lsymbol* temp = Lhead;
    printf("---- Local Symbol Table: %s ----\n", fname);
    printf("%-15s %-7s %-7s\n", "name", "type", "binding");
    while(temp != NULL) {
        printf("%-15s %-7d BP%+d\n", temp->name, temp->type, temp->binding);
        temp = temp->next;
    }
}

/* ---------------- Type table ---------------- */

static Typetable* ttable[MAX_TYPES];
static int tcount = 0;

int TCount() {
    return tcount;
}

int TLookup(char* name) {
    int i;
    for(i = 0; i < tcount; i++) {
        if(strcmp(ttable[i]->name, name) == 0) return ttable[i]->code;
    }
    return -1;
}

int TCreate(char* name) {
    if(TLookup(name) >= 0 || tcount >= MAX_TYPES) return -1;
    Typetable* t = (Typetable*) malloc(sizeof(Typetable));
    t->name = strdup(name);
    t->code = USER_TYPE_BASE + tcount;
    t->nfields = 0;
    t->fields = NULL;
    ttable[tcount++] = t;
    return t->code;
}

int TAddField(int code, char* name, int type) {
    Typetable* t = ttable[code - USER_TYPE_BASE];
    Field* f;
    Field* last = NULL;
    for(f = t->fields; f != NULL; f = f->next) {
        if(strcmp(f->name, name) == 0) return 1;
        last = f;
    }
    if(t->nfields >= MAX_FIELDS) return 2;
    f = (Field*) malloc(sizeof(Field));
    f->name = strdup(name);
    f->type = type;
    f->index = t->nfields++;
    f->next = NULL;
    if(last == NULL) t->fields = f; else last->next = f;
    return 0;
}

Field* FLookup(int code, char* name) {
    Field* f;
    if(!isUserType(code)) return NULL;
    for(f = ttable[code - USER_TYPE_BASE]->fields; f != NULL; f = f->next) {
        if(strcmp(f->name, name) == 0) return f;
    }
    return NULL;
}

const char* typeName(int type) {
    switch(type) {
        case INTEGER_TYPE: return "int";
        case STRING_TYPE: return "str";
        case NULL_TYPE: return "null";
        case ALLOC_TYPE: return "alloc";
        case BOOLEAN_TYPE: return "boolean";
        case INTEGER_POINTER_TYPE: return "int*";
        case STRING_POINTER_TYPE: return "str*";
        case INTEGER_ARRAY_TYPE: return "int[]";
        case STRING_ARRAY_TYPE: return "str[]";
    }
    if(isUserType(type) && type - USER_TYPE_BASE < tcount) return ttable[type - USER_TYPE_BASE]->name;
    return "?";
}

void print_type_table() {
    int i;
    Field* f;
    printf("---- Type Table ----\n");
    for(i = 0; i < tcount; i++) {
        printf("%s (code %d, %d fields):", ttable[i]->name, ttable[i]->code, ttable[i]->nfields);
        for(f = ttable[i]->fields; f != NULL; f = f->next) {
            printf(" %s:%s@%d", f->name, typeName(f->type), f->index);
        }
        printf("\n");
    }
}
