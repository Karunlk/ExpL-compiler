#include "task1.h"

Gsymbol *Ghead = NULL;
static int nextFunctionLabel = 0;
static int nextGlobalBinding = 4096;

static void rejectDuplicate(const char *name)
{
    if (lookup(name) != NULL)
    {
        fprintf(stderr, "Error: %s is already declared\n", name);
        exit(EXIT_FAILURE);
    }
}

static const char *typeName(int type)
{
    switch (type)
    {
        case TYPE_INT: return "INT";
        case TYPE_STR: return "STR";
        case TYPE_INT_PTR: return "INT_PTR";
        case TYPE_STR_PTR: return "STR_PTR";
        default: return "UNKNOWN";
    }
}

Gsymbol *lookup(const char *name)
{
    Gsymbol *entry = Ghead;

    while (entry != NULL)
    {
        if (strcmp(entry->name, name) == 0)
            return entry;
        entry = entry->next;
    }

    return NULL;
}

Paramstruct *makeParam(const char *name, int type)
{
    Paramstruct *param = malloc(sizeof(*param));

    if (param == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    param->name = strdup(name);
    if (param->name == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    param->type = type;
    param->next = NULL;
    return param;
}

Paramstruct *appendParam(Paramstruct *head, Paramstruct *param)
{
    Paramstruct *tail;

    if (head == NULL)
        return param;

    tail = head;
    while (tail->next != NULL)
        tail = tail->next;
    tail->next = param;
    return head;
}

void installVariable(const char *name, int type, int size, int rows,
                     int columns, int dimension)
{
    Gsymbol *entry;

    rejectDuplicate(name);

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    entry->name = strdup(name);
    if (entry->name == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    entry->type = type;
    entry->size = size;
    entry->rows = rows;
    entry->columns = columns;
    entry->dimension = dimension;
    entry->binding = nextGlobalBinding;
    nextGlobalBinding += size;
    entry->flabel = -1;
    entry->next = Ghead;
    Ghead = entry;
}

void installFunction(const char *name, int type, Paramstruct *paramlist)
{
    Gsymbol *entry;

    rejectDuplicate(name);

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    entry->name = strdup(name);
    if (entry->name == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    entry->type = type;
    entry->paramlist = paramlist;
    entry->flabel = nextFunctionLabel++;
    entry->binding = -1;
    entry->next = Ghead;
    Ghead = entry;
}

void checkFunctionDefinition(const char *name, int type, Paramstruct *paramlist)
{
    Gsymbol *function = lookup(name);
    Paramstruct *declared;
    Paramstruct *defined;

    if (strcmp(name, "main") == 0)
    {
        if (type != TYPE_INT || paramlist != NULL)
        {
            fprintf(stderr, "Error: main must be declared as int main()\n");
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (function == NULL || function->flabel < 0)
    {
        fprintf(stderr, "Error: function %s has no declaration\n", name);
        exit(EXIT_FAILURE);
    }
    if (function->defined)
    {
        fprintf(stderr, "Error: function %s is defined more than once\n", name);
        exit(EXIT_FAILURE);
    }
    if (function->type != type)
    {
        fprintf(stderr, "Error: return type mismatch for function %s\n", name);
        exit(EXIT_FAILURE);
    }
    declared = function->paramlist;
    defined = paramlist;
    while (declared != NULL && defined != NULL)
    {
        if (declared->type != defined->type || strcmp(declared->name, defined->name) != 0)
        {
            fprintf(stderr, "Error: parameter name or type mismatch for function %s\n", name);
            exit(EXIT_FAILURE);
        }
        declared = declared->next;
        defined = defined->next;
    }
    if (declared != NULL || defined != NULL)
    {
        fprintf(stderr, "Error: parameter count mismatch for function %s\n", name);
        exit(EXIT_FAILURE);
    }
    function->defined = 1;
}

void printGsymbol(void)
{
    Gsymbol *entry;

    printf("Global Symbol Table\n");
    printf("Name\tKind\t\tType\tSize\tBinding\tFLabel\tParameters\n");

    for (entry = Ghead; entry != NULL; entry = entry->next)
    {
        Paramstruct *param;

        if (entry->paramlist != NULL || entry->flabel >= 0)
        {
            printf("%s\tFUNCTION\t%s\t-\t-\tF%d\t",
                   entry->name, typeName(entry->type), entry->flabel);
            for (param = entry->paramlist; param != NULL; param = param->next)
            {
                printf("%s %s", typeName(param->type), param->name);
                if (param->next != NULL)
                    printf(", ");
            }
            printf("\n");
        }
        else
        {
            printf("%s\tVARIABLE\t%s\t%d\t%d\t-\t-\n",
                   entry->name, typeName(entry->type), entry->size,
                   entry->binding);
        }
    }
}

void freeGsymbol(void)
{
    Gsymbol *entry = Ghead;

    while (entry != NULL)
    {
        Gsymbol *next = entry->next;
        Paramstruct *param = entry->paramlist;

        while (param != NULL)
        {
            Paramstruct *nextParam = param->next;
            free(param->name);
            free(param);
            param = nextParam;
        }

        free(entry->name);
        free(entry);
        entry = next;
    }

    Ghead = NULL;
    nextGlobalBinding = 4096;
}
