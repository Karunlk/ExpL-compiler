#! /bin/bash

CC="gcc"

STAGE="stage4"
TASK="exe2"
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

rm -rf lex.yy.c y.tab.c y.tab.h a.out output.o label_translate