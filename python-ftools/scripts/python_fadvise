#!/usr/bin/env python

import ftools
import os
import sys
import optparse

def main():

    parser = optparse.OptionParser(description="Advise the Linux kernel to manipulate filesystem page cache for files or directories")
    parser.add_option('-m', '--mode', dest='mode', default=None,metavar='MODE', nargs=1,
                      help="""The modes are:
normal - No special treatment.
random - Expect page references in random order.
sequential - Expect page references in sequential order.
willneed - Expect access in the near future.
dontneed - Do not expect access in the near future. Subsequent access of pages in this range will succeed, but will result either in reloading of the memory contents from the underlying mapped file or zero-fill-in-demand pages for mappings without an underlying file.
noreuse - NOTE: This mode is currently a no-op! Access data only once.""")

    parser.add_option('-d', '--directory', dest='directory', default=None,
                      help="Recursively descend into a directory")

    options, args = parser.parse_args()

    mode_dict = { 'normal':'POSIX_FADV_NORMAL',
                'random':'POSIX_FADV_RANDOM',
                'sequential':'POSIX_FADV_SEQUENTIAL',
                'willneed':'POSIX_FADV_WILLNEED',
                'dontneed':'POSIX_FADV_DONTNEED',
                'noreuse':'POSIX_FADV_NOREUSE' }

    if options.mode == None:
        parser.print_help()
        return 1

    if len(args) == 0 and options.directory == None:
        parser.print_help()
        return 1

    mode = mode_dict[options.mode.lower()]

    if options.directory:
        for (path, dirs, files) in os.walk(options.directory):
            for myfile in files:
                fd = file(os.path.join(path,myfile),'r')
                ftools.fadvise(fd.fileno(),mode=mode)
                fd.close()

    for f in args:
        fd = file(f, 'r')
        ftools.fadvise(fd.fileno(),mode=mode)
        fd.close()

if __name__ == '__main__':
    sys.exit(main())
