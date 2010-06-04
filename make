#!/bin/sh

# simple/stupid make until I setup a Makefile

DIR=/usr/bin/

if [ "$1" == "" ]; then

    gcc src/c/fadvise.c -o fadvise
    gcc src/c/fallocate.c -o fallocate
    gcc src/c/fincore.c -o fincore
    gcc src/c/showrlimit.c -o showrlimit
    #gcc src/c/waste_memory.c -o waste_memory

fi

if [ "$1" == "install" ]; then

    cp fadvise $DIR
    cp fallocate $DIR
    cp fincore $DIR
    cp showrlimit $DIR
    #gcc src/c/waste_memory.c -o waste_memory

fi

# FIXME make a deb now.