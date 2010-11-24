#!/usr/bin/env python

import ftools
import sys
import time
import stat
import os

def main():
    if len(sys.argv) < 2:
        print 'Usage: %s <filename>' % sys.argv[0]
        return

    length = 60
    ftools.fallocate(path=sys.argv[1], length=length)

if __name__ == '__main__':
    main()
