%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "exe2.h"
    #include "exe2.c"
    
    int yylex(void);
    void yyerror(const char *s);
    FILE *fptr;
%}

%union {
    struct tnode *node;
    int integer;
}

%token <integer> NUM
%type <node> expr
%left '+' '-'
%left '*' '/'

%%
start: expr '\n'   { fptr = fopen("code.xsm","w");
                     if(!fptr){ perror("fopen"); exit(1); }
                     fprintf(fptr, "0\n2056\n0\n0\n0\n0\n0\n0\n");
                     int tmp = codeGen($1,fptr);
                     
                     fprintf(fptr, "MOV [4096], R%d\n",tmp);
                     fprintf(fptr, "MOV R0, R%d\n",tmp);
                     
                     fprintf(fptr,
                     "MOV SP, 4096\n"
                     "MOV R1, \"Write\"\n"
                     "PUSH R1\n"
                     "MOV R1, -2\n"
                     "PUSH R1\n"
                     "PUSH R0\n"
                     "PUSH R1\n"
                     "PUSH R1\n"
                     "CALL 0\n"
                     "POP R0\n"
                     "POP R1\n"
                     "POP R1\n"
                     "POP R1\n"
                     "POP R1\n"
                     "MOV R1, \"Exit\"\n"
                     "PUSH R1\n"
                     "MOV R1, -2\n"
                     "PUSH R1\n"
                     "PUSH R1\n"
                     "PUSH R1\n"
                     "PUSH R1\n"
                     "BRKP\n"
                     "CALL 0\n"
                     );
                     fclose(fptr);
                     exit(0);
                     }
    ;

expr: expr '+' expr {$$ = makeOperatorNode('+',$1,$3);}
    | expr '-' expr {$$ = makeOperatorNode('-',$1,$3);}
    | expr '*' expr {$$ = makeOperatorNode('*',$1,$3);}
    | expr '/' expr {$$ = makeOperatorNode('/',$1,$3);}
    | '(' expr ')'  {$$=$2;}
    | NUM           {$$ = makeLeafNode($1);}
    ;
%%

void yyerror(const char *s)
    {
        fprintf(stderr, "Error: %s\n",s);
    }

int main(){
    yyparse();
    return 0;
}