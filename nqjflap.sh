#!/bin/bash

export PATH=$PATH:./bin/

NAME=`basename $1`

echo "Compiling JFLAP to /tmp/$NAME.c"
./jflap2nqc.rb "$1" "/tmp/$NAME".c
echo "Done, pushing to RCX..."
nqc -Trcx2 -d "/tmp/$NAME".c 
echo "Done!"
