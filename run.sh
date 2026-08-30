#! /bin/bash

STAGE="stage4"
TASK="exe1"
XSMADDRESS="$HOME/Documents/compiler-lab/xsm_expl"
ADDRESS="/$STAGE/$TASK"

cd $XSMADDRESS
./xsm -l library.lib -e ../$ADDRESS/output.xsm 
rm -rf ../$ADDRESS/output.xsm