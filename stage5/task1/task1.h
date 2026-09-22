#ifndef TASK1_H
#define TASK1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_INT 1
#define TYPE_STR 2
#define TYPE_INT_PTR 3
#define TYPE_STR_PTR 4

typedef struct Paramstruct {
    char *name;
    int type;
    struct Paramstruct *next;
} Paramstruct;

typedef struct Gsymbol {
    char *name;
    int type;
    int size;
    int rows;
    int columns;
    int dimension;
    int binding;
    Paramstruct *paramlist;
    int flabel;
    struct Gsymbol *next;
} Gsymbol;

extern Gsymbol *Ghead;

Gsymbol *lookup(const char *name);
void installVariable(const char *name, int type, int size, int rows,
                     int columns, int dimension);
void installFunction(const char *name, int type, Paramstruct *paramlist);
Paramstruct *makeParam(const char *name, int type);
Paramstruct *appendParam(Paramstruct *head, Paramstruct *param);
void printGsymbol(void);
void freeGsymbol(void);

#endif
