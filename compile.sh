#! /bin/bash

CC="gcc"

ADDRESS="$HOME/Documents/compiler-lab/stage4/task2/"
LEXFILE="task2.l"
YACCFILE="task2.y"
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