#include "task2.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern void yyerror(const char *s);
int genLibCall(char* code, int arg1_reg, int arg1_const, FILE* target_file);

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
    newNode->Ctype = NULL;

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
    if(isClassType(base->type)) {
        ClassField *cf = CFieldLookup(CByCode(base->type), field);
        /* A member followed by '(' is reduced as FieldRef first; defer the
           method-vs-field decision to makeMethodCall. */
        if(cf == NULL) return createNode(0, base->type, FIELD_NODE, field, base, NULL, NULL);
        return createNode(cf->index, cf->type, FIELD_NODE, field, base, NULL, NULL);
    }
    if(!isUserType(base->type)) semanticError("%s is not a variable of a user-defined type", base->varname);
    Field* f = FLookup(base->type, field);
    if(f == NULL) semanticError("no member field named %s", field);
    return createNode(f->index, f->type, FIELD_NODE, field, base, NULL, NULL);
}

tnode* makeMethodCall(tnode* receiver, char* name, tnode* args) {
    ClassMethod* m = CMethodLookup(CByCode(receiver->type), name);
    Gsymbol* g;
    tnode* node;
    if(m == NULL) semanticError("unknown method %s", name);
    g = calloc(1, sizeof(*g));
    g->name = strdup(name);
    g->flabel = m->flabel;
    g->type = m->type;
    g->paramlist = m->params;
    node = createNode(0, m->type, METHOD_CALL_NODE, name, args, receiver, NULL);
    node->Gentry = g;
    return node;
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

/* Computes the memory address of a variable (or array element) into a register */
int getVarAddressReg(tnode *varNode, FILE *target_file) {
    // Member field: address = (value of the base reference) + field offset
    if(varNode->nodetype == FIELD_NODE) {
        int base_reg = codeGen(varNode->left, target_file);
        if(varNode->val != 0) fprintf(target_file, "ADD R%d, %d\n", base_reg, varNode->val);
        return base_reg;
    }

    int addr_reg = getReg();

    // Local variable / parameter: address = BP + binding
    if(varNode->Lentry != NULL) {
        int b = varNode->Lentry->binding;
        fprintf(target_file, "MOV R%d, BP\n", addr_reg);
        if(b >= 0) fprintf(target_file, "ADD R%d, %d\n", addr_reg, b);
        else fprintf(target_file, "SUB R%d, %d\n", addr_reg, -b);
        return addr_reg;
    }

    int base_addr = varNode->Gentry->binding;
    fprintf(target_file, "MOV R%d, %d\n", addr_reg, base_addr);

    // 1D Array: index = left
    if(varNode->left != NULL && varNode->right == NULL) {
        int off_reg = codeGen(varNode->left, target_file);
        fprintf(target_file, "ADD R%d, R%d\n", addr_reg, off_reg);
        freeReg();
    }

    // 2D Array
    else if(varNode->left != NULL && varNode->right != NULL) {
        int r_idx = codeGen(varNode->left, target_file);
        int c_idx = codeGen(varNode->right, target_file);
        int cols = varNode->Gentry->metadata[1];

        fprintf(target_file, "MUL R%d, %d\n", r_idx, cols);
        fprintf(target_file, "ADD R%d, R%d\n", r_idx, c_idx);
        fprintf(target_file, "ADD R%d, R%d\n", addr_reg, r_idx);

        freeReg();
        freeReg();
    }

    return addr_reg;
}

/* Address of anything that can be assigned to / read into / have its address taken */
int getLValueAddr(tnode* node, FILE* target_file) {
    if(node->nodetype == DEREF_NODE) return codeGen(node->left, target_file);
    return getVarAddressReg(node, target_file);
}

/* Push arguments in reverse order: the last argument is pushed first,
   so that argument 1 ends up at [BP-3], argument 2 at [BP-4], ... */
void pushArgs(tnode* arg, FILE* target_file) {
    if(arg == NULL) return;
    pushArgs(arg->right, target_file);
    int r = codeGen(arg->left, target_file);
    fprintf(target_file, "PUSH R%d\n", r);
    freeReg();
}

int genFunctionCall(tnode* t, FILE* target_file) {
    int saved = reg_index;
    int i;

    // 1. caller saves registers in use
    for(i = 0; i <= saved; i++) {
        fprintf(target_file, "PUSH R%d\n", i);
    }

    // 2. Push explicit arguments first.  The implicit self reference must be
    // closest to the return address so it is visible at BP-3.
    pushArgs(t->left, target_file);
    if(t->nodetype == METHOD_CALL_NODE && t->right != NULL) {
        int self_reg = codeGen(t->right, target_file);
        fprintf(target_file, "PUSH R%d\n", self_reg);
        freeReg();
    }

    // 4. push space for the return value
    int tmp = getReg();
    fprintf(target_file, "PUSH R%d\n", tmp);
    freeReg();

    // 5. call
    fprintf(target_file, "CALL F%d\n", t->Gentry->flabel);

    // 6. after return: fetch return value into a fresh register
    int ret_reg = getReg();
    fprintf(target_file, "POP R%d\n", ret_reg);

    // 7. pop the arguments
    int nargs = countParams(t->Gentry->paramlist);
    if(t->nodetype == METHOD_CALL_NODE) nargs++;
    if(nargs > 0) {
        tmp = getReg();
        for(i = 0; i < nargs; i++) {
            fprintf(target_file, "POP R%d\n", tmp);
        }
        freeReg();
    }

    // 8. restore saved registers
    for(i = saved; i >= 0; i--) {
        fprintf(target_file, "POP R%d\n", i);
    }

    return ret_reg;
}

/* Calls a library function through the ABI: CALL 0 with the function code, three arguments and an
   empty slot for the return value on the stack (the same interface as Read/Write/Exit).
   Registers in use are saved around the call. Returns a fresh register holding the return value. */
int genLibCall(char* code, int arg1_reg, int arg1_const, FILE* target_file) {
    int saved = reg_index;
    int i;
    for(i = 0; i <= saved; i++) fprintf(target_file, "PUSH R%d\n", i);
    int t = getReg();
    int res = getReg();
    fprintf(target_file, "MOV R%d, \"%s\"\n", t, code);
    fprintf(target_file, "PUSH R%d\n", t);               // function code
    if(arg1_reg >= 0) {
        fprintf(target_file, "PUSH R%d\n", arg1_reg);    // argument 1
    } else {
        fprintf(target_file, "MOV R%d, %d\n", t, arg1_const);
        fprintf(target_file, "PUSH R%d\n", t);
    }
    fprintf(target_file, "PUSH R%d\n", t);               // argument 2 (unused)
    fprintf(target_file, "PUSH R%d\n", t);               // argument 3 (unused)
    fprintf(target_file, "PUSH R%d\n", t);               // space for the return value
    fprintf(target_file, "CALL 0\n");
    fprintf(target_file, "POP R%d\n", res);              // return value
    for(i = 0; i < 4; i++) fprintf(target_file, "POP R%d\n", t);
    for(i = saved; i >= 0; i--) fprintf(target_file, "POP R%d\n", i);
    fprintf(target_file, "MOV R%d, R%d\n", t, res);
    freeReg();                                           // res
    return t;
}

void genReturnSequence(FILE* target_file) {
    fprintf(target_file, "MOV SP, BP\n");
    fprintf(target_file, "POP BP\n");
    fprintf(target_file, "RET\n");
}

int codeGen(tnode *t, FILE *target_file) {
    if (t == NULL) return -1;
    if (t->nodetype == STATEMENT) {
        codeGen(t->left, target_file);
        codeGen(t->right, target_file);
        return -1;
    }

    if (t->nodetype == CONSTANT) {
        int r = getReg();
        fprintf(target_file, "MOV R%d, %d\n", r, t->val);
        return r;
    }

    if(t->nodetype == DEREF_NODE) {
        int reg = codeGen(t->left, target_file);
        fprintf(target_file, "MOV R%d, [R%d]\n", reg, reg);
        return reg;
    }

    if(t->nodetype == ADDRESS_NODE) {
        return getLValueAddr(t->left, target_file);
    }

    if (t->nodetype == VARIABLE) {
        int r = getReg();
        int addr_reg = getVarAddressReg(t, target_file);
        fprintf(target_file, "MOV R%d, [R%d]\n", r, addr_reg);
        freeReg(); // free addr_reg
        return r;
    }

    if(t->nodetype == FIELD_NODE) {
        int r = getVarAddressReg(t, target_file);
        fprintf(target_file, "MOV R%d, [R%d]\n", r, r);
        return r;
    }

    if(t->nodetype == ALLOC_NODE) {
        return genLibCall("Alloc", -1, 8, target_file);        // size = fixed block size
    }

    if(t->nodetype == INIT_NODE) {
        return genLibCall("Heapset", -1, 0, target_file);
    }

    if(t->nodetype == FREE_NODE) {
        int a = codeGen(t->left, target_file);
        int r = genLibCall("Free", a, 0, target_file);
        fprintf(target_file, "MOV R%d, R%d\n", a, r);
        freeReg();
        return a;
    }

    if(t->nodetype == METHOD_CALL_NODE) {
        return genFunctionCall(t, target_file);
    }
    if(t->nodetype == FUNC_CALL_NODE) {
        return genFunctionCall(t, target_file);
    }

    if(t->nodetype == RETURN_NODE) {
        int r = codeGen(t->left, target_file);
        int addr = getReg();
        // return value goes to [BP-2]
        fprintf(target_file, "MOV R%d, BP\n", addr);
        fprintf(target_file, "SUB R%d, 2\n", addr);
        fprintf(target_file, "MOV [R%d], R%d\n", addr, r);
        freeReg();
        freeReg();
        genReturnSequence(target_file);
        return -1;
    }

    if(t->nodetype == IF_NODE) {
        int regno = codeGen(t->left, target_file);
        int else_label = getLabel();
        int end_label = getLabel();

        fprintf(target_file, "JZ R%d, L%d\n", regno, else_label);
        freeReg();

        if(t->mid != NULL) codeGen(t->mid, target_file);
        fprintf(target_file, "JMP L%d\n", end_label);

        fprintf(target_file, "L%d:\n", else_label);
        if(t->right != NULL) codeGen(t->right, target_file);

        fprintf(target_file, "L%d:\n", end_label);
        return -1;
    }

    if(t->nodetype == WHILE_NODE) {
        int while_start = getLabel();
        int while_end = getLabel();

        fprintf(target_file, "L%d:\n", while_start);
        int reg_no = codeGen(t->left, target_file);
        fprintf(target_file, "JZ R%d, L%d\n", reg_no, while_end);
        freeReg();

        loop_top++;
        continue_labels[loop_top] = while_start;
        break_labels[loop_top] = while_end;

        codeGen(t->right, target_file);

        loop_top--;

        fprintf(target_file, "JMP L%d\n", while_start);
        fprintf(target_file, "L%d:\n", while_end);
        return -1;
    }

    if(t->nodetype == DO_WHILE_NODE) {
        int do_start = getLabel();
        int do_continue = getLabel();
        int do_end = getLabel();

        fprintf(target_file, "L%d:\n", do_start);

        loop_top++;
        continue_labels[loop_top] = do_continue;
        break_labels[loop_top] = do_end;

        codeGen(t->right, target_file);
        loop_top--;

        fprintf(target_file, "L%d:\n", do_continue);
        int reg_no = codeGen(t->left, target_file);
        fprintf(target_file, "JZ R%d, L%d\n", reg_no, do_end);
        freeReg();

        fprintf(target_file, "JMP L%d\n", do_start);
        fprintf(target_file, "L%d:\n", do_end);
        return -1;
    }

    if(t->nodetype == REPEAT_UNTIL_NODE) {
        int repeat_start = getLabel();
        int repeat_continue = getLabel();
        int repeat_end = getLabel();

        fprintf(target_file, "L%d:\n", repeat_start);

        loop_top++;
        continue_labels[loop_top] = repeat_continue;
        break_labels[loop_top] = repeat_end;

        codeGen(t->right, target_file);
        loop_top--;

        fprintf(target_file, "L%d:\n", repeat_continue);
        int reg_no = codeGen(t->left, target_file);
        fprintf(target_file, "JZ R%d, L%d\n", reg_no, repeat_start);
        freeReg();

        fprintf(target_file, "L%d:\n", repeat_end);
        return -1;
    }

    if (t->nodetype == ASSIGNMENT) {
        int right_reg = codeGen(t->right, target_file);
        int addr_reg = getLValueAddr(t->left, target_file);
        fprintf(target_file, "MOV [R%d], R%d\n", addr_reg, right_reg);
        freeReg(); // free addr_reg
        freeReg(); // free right_reg
        return -1;
    }

    if(t->nodetype == BREAK_NODE) {
        if(loop_top >= 0) {
            fprintf(target_file, "JMP L%d\n", break_labels[loop_top]);
        }
        return -1;
    }

    if(t->nodetype == CONTINUE_NODE) {
        if(loop_top >= 0) {
            fprintf(target_file, "JMP L%d\n", continue_labels[loop_top]);
        }
        return -1;
    }

    if(t->nodetype == WRITE_NODE) {
        int expr_reg = codeGen(t->left, target_file);
        int temp = getReg();

        fprintf(target_file, "MOV R%d, \"Write\"\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "MOV R%d, -2\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "MOV R%d, R%d\n", temp, expr_reg);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);

        fprintf(target_file, "CALL 0\n");

        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);

        freeReg();  // Free temp
        freeReg();  // Free expr_reg
        return -1;
    }

    if(t->nodetype == READ_NODE) {
        int temp = getReg();
        int addr_reg = getLValueAddr(t->left, target_file);

        fprintf(target_file, "MOV R%d, \"Read\"\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "MOV R%d, -1\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "MOV R%d, R%d\n", temp, addr_reg);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);
        fprintf(target_file, "PUSH R%d\n", temp);

        fprintf(target_file, "CALL 0\n");

        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);
        fprintf(target_file, "POP R%d\n", temp);

        freeReg(); // free addr_reg
        freeReg(); // free temp
        return -1;
    }

    if(t->nodetype == STRING_CONSTANT_NODE) {
        int reg = getReg();
        fprintf(target_file, "MOV R%d, %s\n", reg, t->varname);
        return reg;
    }

    if(t->nodetype == NOT_NODE) {
        int reg = codeGen(t->left, target_file);
        int zero = getReg();
        fprintf(target_file, "MOV R%d, 0\n", zero);
        fprintf(target_file, "EQ R%d, R%d\n", reg, zero);
        freeReg();
        return reg;
    }

    int left_reg = codeGen(t->left, target_file);
    int right_reg = codeGen(t->right, target_file);

    switch(t->nodetype) {
        case PLUS_NODE:
            fprintf(target_file, "ADD R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case MINUS_NODE:
            fprintf(target_file, "SUB R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case MUL_NODE:
            fprintf(target_file, "MUL R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case DIV_NODE:
            fprintf(target_file, "DIV R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case GT_NODE:
            fprintf(target_file, "GT R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case EQ_NODE:
            fprintf(target_file, "EQ R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case GE_NODE:
            fprintf(target_file, "GE R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case LT_NODE:
            fprintf(target_file, "LT R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case LE_NODE:
            fprintf(target_file, "LE R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case NE_NODE:
            fprintf(target_file, "NE R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case AND_NODE:
            // booleans are 0/1: a AND b == a * b
            fprintf(target_file, "MUL R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case OR_NODE:
            // a OR b == (a + b) > 0
            fprintf(target_file, "ADD R%d, R%d\n", left_reg, right_reg);
            fprintf(target_file, "MOV R%d, 0\n", right_reg);
            fprintf(target_file, "GT R%d, R%d\n", left_reg, right_reg);
            freeReg(); return left_reg;
        case MODULUS_NODE: {
            // a % b is a-(a/b)*b
            int temp_reg = getReg();
            fprintf(target_file, "MOV R%d, R%d\n", temp_reg, left_reg);
            fprintf(target_file, "DIV R%d, R%d\n", temp_reg, right_reg);
            fprintf(target_file, "MUL R%d, R%d\n", temp_reg, right_reg);
            fprintf(target_file, "SUB R%d, R%d\n", left_reg, temp_reg);
            freeReg();
            freeReg();
            return left_reg;
        }
    }
    return -1;
}

/* Header + code that sets up the stack, calls main and exits.
   Emitted once the global declarations are processed. */
void genStartup(FILE* fp) {
    fprintf(fp, "0\n2056\n0\n0\n0\n0\n0\n0\n");
    fprintf(fp, "MOV SP, %d\n", get_stack_pointer());
    fprintf(fp, "MOV BP, SP\n");
    fprintf(fp, "PUSH R0\n");          // space for main's return value
    fprintf(fp, "CALL MAIN\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "MOV R0, \"Exit\"\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "CALL 0\n");
}

/* Callee side of the calling convention */
void genFunction(char* label, int nlocals, tnode* body, FILE* fp) {
    int i;
    reg_index = -1;
    loop_top = -1;

    fprintf(fp, "%s:\n", label);
    fprintf(fp, "PUSH BP\n");
    fprintf(fp, "MOV BP, SP\n");
    for(i = 0; i < nlocals; i++) {
        fprintf(fp, "PUSH R0\n");      // space for local variables
    }

    codeGen(body, fp);

    // in case control reaches the end of the function without a return
    genReturnSequence(fp);
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- Global symbol table ---------------- */

Gsymbol* head = NULL;
Gsymbol* tail = NULL;
int bind_start = 4096;
int flabel_count = 0;
static Classtable* chead = NULL;
static Classtable* ctail = NULL;
static int ccount = 0;

static ClassField* copyFields(ClassField* f) {
    ClassField *head = NULL, *tail = NULL;
    for(; f; f=f->next) { ClassField* n=calloc(1,sizeof(*n)); *n=*f; n->name=strdup(f->name); n->next=NULL; if(!head) head=n; else tail->next=n; tail=n; }
    return head;
}
static ClassMethod* copyMethods(ClassMethod* m) {
    ClassMethod *head = NULL, *tail = NULL;
    for(; m; m=m->next) { ClassMethod* n=calloc(1,sizeof(*n)); *n=*m; n->name=strdup(m->name); n->next=NULL; if(!head) head=n; else tail->next=n; tail=n; }
    return head;
}
Classtable* CLookup(char* name) { Classtable* c; for(c=chead;c&&strcmp(c->name,name);c=c->next); return c; }
Classtable* CByCode(int code) { Classtable* c; for(c=chead;c&&c->code!=code;c=c->next); return c; }
Classtable* CInstall(char* name, char* parent) {
    Classtable* p = parent ? CLookup(parent) : NULL;
    if(CLookup(name) || (parent && !p)) return NULL;
    Classtable* c=calloc(1,sizeof(*c)); c->name=strdup(name); c->code=CLASS_TYPE_BASE+ccount; c->class_index=ccount++;
    c->parent=p; if(p) { c->fields=copyFields(p->fields); c->methods=copyMethods(p->methods); c->nfields=p->nfields; c->nmethods=p->nmethods; }
    if(!chead) chead=c; else ctail->next=c; ctail=c; return c;
}
ClassField* CFieldLookup(Classtable* c,char* n) { ClassField* f; for(f=c?c->fields:NULL;f&&strcmp(f->name,n);f=f->next); return f; }
ClassMethod* CMethodLookup(Classtable* c,char* n) { ClassMethod* m; for(m=c?c->methods:NULL;m&&strcmp(m->name,n);m=m->next); return m; }
int CAddField(Classtable* c,char* n,int t) { ClassField *f,*q; if(CFieldLookup(c,n)) return 1; if(c->nfields>=8)return 2; f=calloc(1,sizeof(*f)); f->name=strdup(n);f->type=t;f->index=c->nfields++; for(q=c->fields;q&&q->next;q=q->next);if(q)q->next=f;else c->fields=f;return 0; }
int CAddMethod(Classtable* c,char* n,int t,Paramstruct* p) { ClassMethod *m,*q=CMethodLookup(c,n); if(q){ if(q->defined)return 1; q->type=t;q->params=p;q->flabel=flabel_count++;q->defined=1;return 0;} if(c->nmethods>=8)return 2; m=calloc(1,sizeof(*m));m->name=strdup(n);m->type=t;m->params=p;m->index=c->nmethods++;m->flabel=flabel_count++;m->defined=1;for(q=c->methods;q&&q->next;q=q->next);if(q)q->next=m;else c->methods=m;return 0; }
int CIsDescendant(Classtable* c,Classtable* p){ for(;c;c=c->parent)if(c==p)return 1;return 0; }
int CCount(void){return ccount;}
void CEmitVTables(FILE* fp){ Classtable*c;ClassMethod*m;for(c=chead;c;c=c->next)for(m=c->methods;m;m=m->next)fprintf(fp,"MOV [R0], F%d\\n",m->flabel); }

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

int LInstallMethodParams(Paramstruct* params) {
    int binding = -4;
    while(params != NULL) {
        if(LAppend(params->name, params->type, binding)) return 1;
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
        case BOOLEAN_TYPE: return "boolean";
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
