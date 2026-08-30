#! /bin/bash

XSMADDRESS="$HOME/Documents/compiler-lab/xsm_expl"
ADDRESS="/stage4/task2"

cd $XSMADDRESS
./xsm -l library.lib -e ../$ADDRESS/output.xsm 
rm -rf ../$ADDRESS/output.xsm