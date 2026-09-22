#!/bin/bash
set -euo pipefail

CC="gcc"
STAGE="stage5"
TASK="${1:-exe1}"
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ADDRESS="$ROOT_DIR/$STAGE/$TASK"
LEXFILE="$TASK.l"
YACCFILE="$TASK.y"
INPUTFILE="input.expl"

if [[ ! -f "$ADDRESS/$LEXFILE" || ! -f "$ADDRESS/$YACCFILE" || ! -f "$ADDRESS/$INPUTFILE" ]]; then
    echo "Unknown or incomplete target: $STAGE/$TASK" >&2
    exit 1
fi

cd "$ADDRESS"
lex "$LEXFILE"
yacc -d "$YACCFILE"
"$CC" lex.yy.c y.tab.c -lfl -o a.out
./a.out "$INPUTFILE"

if [[ -f output.o ]]; then
    lex label_translate.l
    "$CC" lex.yy.c -o label_translate
    ./label_translate output.o

    cd "$ROOT_DIR/xsm_expl"
    xsm_output="$(./xsm -l library.lib -e "../$STAGE/$TASK/output.xsm" 2>&1)"
    printf '%s\n' "$xsm_output"
    if [[ "$xsm_output" == *"Exception"* ]]; then
        echo "XSM reported a runtime exception for $STAGE/$TASK" >&2
        exit 1
    fi
fi

cd "$ADDRESS"
rm -f lex.yy.c y.tab.c y.tab.h a.out output.o output.xsm label_translate
