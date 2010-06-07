#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([linux-ftools], [1.0.0], BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([foo.c])
AM_INIT_AUTOMAKE([linux-ftools], [1.0.0])
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lgthread':
AC_CHECK_LIB([gthread], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
