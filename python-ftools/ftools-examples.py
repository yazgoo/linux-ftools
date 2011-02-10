#!/usr/bin/env python

import ftools
import sys
import time

def main():
    if len(sys.argv) < 2:
        print 'Usage: %s <filename>' % sys.argv[0]
        return

    pages_cached = 0
    pages_total = 0

    fd = file(sys.argv[1], 'r')
    ftools.fadvise(fd.fileno(),mode="POSIX_FADV_WILLNEED")
    print "fadvise POSIX_FADV_WILLNEED"
    print "sleeping..."
    time.sleep(1)

    pages_cached, pages_total = ftools.fincore_ratio(fd.fileno())
    print "fincore_ratio: %i of %i pages cached (%.00f%%)" % (pages_cached, pages_total, (float(pages_cached) / float(pages_total)) * 100.0)

    ftools.fadvise(fd.fileno(),mode="POSIX_FADV_DONTNEED")
    print "fadvise POSIX_FADV_DONTNEED"

    pages_cached, pages_total = ftools.fincore_ratio(fd.fileno())
    print "fincore_ratio: %i of %i pages cached (%.00f%%)" % (pages_cached, pages_total, (float(pages_cached) / float(pages_total)) * 100.0)

    ftools.fadvise(fd.fileno(),mode="POSIX_FADV_WILLNEED")
    print "fadvise POSIX_FADV_WILLNEED"

    print "sleeping..."
    time.sleep(1)
 
    pages_cached, pages_total = ftools.fincore_ratio(fd.fileno())
    fd.close()
    print "fincore_ratio: %i of %i pages cached (%.00f%%)" % (pages_cached, pages_total, (float(pages_cached) / float(pages_total)) * 100.0)


if __name__ == '__main__':
    main()
