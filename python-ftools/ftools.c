#include <Python.h>
#include <linux-ftools.h>

static PyObject *ftools_fincore(PyObject *self, PyObject *args) {
    PyObject *ret;
    int fd;
    void *file_mmap;
    unsigned char *mincore_vec;
    struct stat file_stat;
    ssize_t page_size = getpagesize();
    ssize_t vec_size;

    if(!PyArg_ParseTuple(args, "i", &fd)) {
        return NULL;
    }

    if(fstat(fd, &file_stat) < 0) {
        PyErr_SetString(PyExc_IOError, "Could not fstat file");
        return NULL;
    }

    if ( file_stat.st_size == 0 ) {
        PyErr_SetString(PyExc_IOError, "Cannot mmap zero size file");
        return NULL;
    }

    file_mmap = mmap((void *)0, file_stat.st_size, PROT_NONE, MAP_SHARED, fd, 0);

    if(file_mmap == MAP_FAILED) {
        PyErr_SetString(PyExc_IOError, "Could not mmap file");
        return NULL;
    }

    vec_size = (file_stat.st_size + page_size - 1) / page_size;
    mincore_vec = calloc(1, vec_size);

    if(mincore_vec == NULL) {
        return PyErr_NoMemory();
    }

    if(mincore(file_mmap, file_stat.st_size, mincore_vec) != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        PyErr_SetString(PyExc_OSError, "Could not call mincore for file");
        free(mincore_vec);
        munmap(file_mmap, file_stat.st_size);
        return NULL;
    }

    ret = Py_BuildValue("s#", mincore_vec, vec_size);
    free(mincore_vec);
    munmap(file_mmap, file_stat.st_size);
    return ret;
}

static PyObject *ftools_fincore_ratio(PyObject *self, PyObject *args) {
    int fd;
    void *file_mmap;
    unsigned char *mincore_vec;
    struct stat file_stat;
    ssize_t page_size = getpagesize();
    size_t page_index;
    ssize_t vec_size;

    if(!PyArg_ParseTuple(args, "i", &fd)) {
        return NULL;
    }

    if(fstat(fd, &file_stat) < 0) {
        PyErr_SetString(PyExc_IOError, "Could not fstat file");
        return NULL;
    }

    if ( file_stat.st_size == 0 ) {
        PyErr_SetString(PyExc_IOError, "Cannot mmap zero size file");
        return NULL;
    }

    file_mmap = mmap((void *)0, file_stat.st_size, PROT_NONE, MAP_SHARED, fd, 0);

    if(file_mmap == MAP_FAILED) {
        PyErr_SetString(PyExc_IOError, "Could not mmap file");
        return NULL;
    }

    vec_size = (file_stat.st_size + page_size - 1) / page_size;
    mincore_vec = calloc(1, vec_size);

    if(mincore_vec == NULL) {
        free(mincore_vec);
        munmap(file_mmap, file_stat.st_size);
        return PyErr_NoMemory();
    }

    if(mincore(file_mmap, file_stat.st_size, mincore_vec) != 0) {
        PyErr_SetString(PyExc_OSError, "Could not call mincore for file");
        free(mincore_vec);
        munmap(file_mmap, file_stat.st_size);
        return NULL;
    }

    int cached = 0;
    for (page_index = 0; page_index <= file_stat.st_size/page_size; page_index++) {
        if (mincore_vec[page_index]&1) {
            ++cached;
        }
    }

    free(mincore_vec);
    munmap(file_mmap, file_stat.st_size);

    int total_pages = (int)ceil( (double)file_stat.st_size / (double)page_size );
    return Py_BuildValue("(ii)", cached, total_pages);
}

// ftools.fadvise
static PyObject *ftools_fadvise(PyObject *self, PyObject *args, PyObject *keywds) {
    int fd;
    int offset = 0;
    int length = 0;
    char* str_mode = "";
    char* errstr;
    struct stat file_stat;

    static char *kwlist[] = {"fd","mode", "offset", "length", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "is|ii", kwlist, 
                                     &fd, &str_mode, &offset, &length)) {
        return NULL; 
    }

    int mode = -1;

    if ( strcmp( str_mode , "POSIX_FADV_NORMAL" ) == 0 ) {
        mode = POSIX_FADV_NORMAL;
    } else if ( strcmp( str_mode , "POSIX_FADV_RANDOM" ) == 0 ) {
        mode = POSIX_FADV_RANDOM;
    } else if ( strcmp( str_mode , "POSIX_FADV_SEQUENTIAL" ) == 0 ) {
        mode = POSIX_FADV_SEQUENTIAL;
    } else if ( strcmp( str_mode , "POSIX_FADV_WILLNEED" ) == 0 ) {
        mode = POSIX_FADV_WILLNEED;
    } else if ( strcmp( str_mode , "POSIX_FADV_DONTNEED" ) == 0 ) {
        mode = POSIX_FADV_DONTNEED;
    } else if ( strcmp( str_mode , "POSIX_FADV_NOREUSE" ) == 0 ) {
        mode = POSIX_FADV_NOREUSE;
    } else {
       if( (asprintf(&errstr, "%s is an invalid mode", str_mode)) == -1)
        {   
            return PyErr_NoMemory();
        } 
        PyErr_SetString(PyExc_TypeError, errstr);
        return NULL;
    }

    if(fstat(fd, &file_stat) < 0) {
        PyErr_SetString(PyExc_IOError, "Could not fstat file");
        return NULL;
    }

    if(length == 0) {
        length = file_stat.st_size;
    }

    long result = syscall( SYS_fadvise64, fd, offset, length , mode );

    if ( result != 0 ) {
        if ( result != -1 ) {
            errno=result;
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        } else {
            PyErr_SetString(PyExc_TypeError, "Unable to fadvise");
            return NULL;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}


// ftools.fallocate
static PyObject *ftools_fallocate(PyObject *self, PyObject *args, PyObject *keywds) {
    char* path;
    unsigned long increase;
    fallocate_result result;
    static char *kwlist[] = {"path", "increase", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sk", kwlist,
                                     &path, &increase)) {
        return NULL;
    }

    result = fallocate(path, increase);
    if (result.error_state == FALSE) {
        Py_INCREF(Py_None);
        return Py_None;
    } else {

        char* error_string;
        if( (asprintf(&error_string, "fallocate: %s", result.error_string)) != -1) {
            PyErr_SetString(PyExc_TypeError, error_string);
            return NULL;
        } else {
            return PyErr_NoMemory();
        }
    }
}



static PyMethodDef FtoolsMethods[] = {
    {"fincore", ftools_fincore, METH_VARARGS, "Return the mincore structure for the given file."},
    {"fincore_ratio", ftools_fincore_ratio, METH_VARARGS, "Return a int two tuple indicating file in page cache ratio."},
    {"fadvise", (PyCFunction) ftools_fadvise, METH_KEYWORDS, "fadvise system call for Python!"},
    {"fallocate", (PyCFunction) ftools_fallocate, METH_KEYWORDS, "fallocate system call for Python!"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initftools(void) {
    (void)Py_InitModule("ftools", FtoolsMethods);

}

