#! /bin/bash

CC="gcc"

STAGE="stage5"
TASK="task3"
ADDRESS="$HOME/Documents/compiler-lab/$STAGE/$TASK/"
LEXFILE="$TASK.l"
YACCFILE="$TASK.y"
INPUTFILE="input.expl"

cd $ADDRESS
lex $LEXFILE
yacc -d $YACCFILE
$CC lex.yy.c y.tab.c -lfl
./a.out $INPUTFILE
lex label_translate.l
$CC lex.yy.c -o label_translate
./label_translate output.o

XSMADDRESS="$HOME/Documents/compiler-lab/xsm_expl"
RUNADDRESS="/$STAGE/$TASK"

cd $XSMADDRESS
./xsm -l library.lib -e ../$RUNADDRESS/output.xsm

rm -rf ../$ADDRESS/output.xsm
rm -rf lex.yy.c y.tab.c y.tab.h a.out output.o label_translate