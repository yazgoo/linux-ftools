#!/bin/sh

# simple/stupid make until I setup a Makefile

if [ "$1" == "" ]; then

    gcc src/c/fadvise.c -o fadvise
    gcc src/c/fallocate.c -o fallocate
    gcc src/c/fincore.c -o fincore
    gcc src/c/showrlimit.c -o showrlimit
    #gcc src/c/waste_memory.c -o waste_memory

fi

if [ "$1" == "install" ]; then

    cp fadvise /usr/bin/
    cp fallocate /usr/bin/
    cp fincore /usr/bin/
    cp showrlimit /usr/bin/
    #gcc src/c/waste_memory.c -o waste_memory

fi

# FIXME make a deb now.