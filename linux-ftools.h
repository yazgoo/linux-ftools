#ifndef FTOOLS_H
#define FTOOLS_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/fadvise.h>
#include <linux/falloc.h>
#include <sys/mman.h>
#include <asm/unistd_64.h>


// TODO: READ the @VERSION from configure.ac 
#define LINUX_FTOOLS_VERSION "1.1.0"

#define FALSE 0
#define TRUE 1


typedef struct fallocate_result
{   
    char* error_string;
    long return_value;
    bool error_state;
} fallocate_result;

// need prototypes for the Python module's setup.py's build...
fallocate_result fallocate(char* path, unsigned long length);

#endif
