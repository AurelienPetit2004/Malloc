#!/bin/sh

check()
{
    LD_PRELOAD=./libmalloc.so cat Makefile > file1
    cat Makefile > file2
    diff file1 file2
    if [ $? -eq 0 ]; then
        echo OK
    else
        echo KO
    fi
}

check
