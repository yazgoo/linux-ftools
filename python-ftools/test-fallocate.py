#!/usr/bin/env python

import ftools
import sys
import time
import stat
import os

def main():
    if len(sys.argv) < 3:
        print 'Usage: %s <filename> <length>' % sys.argv[0]
        return

    length = int(sys.argv[2])
    fh = open(sys.argv[1],'w')
    fd = fh.fileno()
    ftools.fallocate(fd=fd, length=length)

if __name__ == '__main__':
    main()
