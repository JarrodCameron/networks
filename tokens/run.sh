#!/usr/bin/env bash

# Author: Jarrod Cameron (z5210220)
# Date:   07/11/19 12:56

if [ ! -z "$1" ]; then
    STRING="$1"
else
    STRING='message john smith is good'
    echo '$STRING ='"$STRING"
fi

gcc tokenise.c -g
valgrind ./a.out "$STRING"
rm -f a.out



