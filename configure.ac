#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(mangoo, 0.2)
AC_CONFIG_SRCDIR([src/lang/c/lib/mangoo.c])
AC_CONFIG_HEADERS([src/lang/c/include/mangooconf.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_PROG_RANLIB

# Checks for header files.
AC_CHECK_HEADERS([malloc.h stdint.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strchr strdup strerror strstr])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/lang/Makefile
src/lang/c/Makefile
src/lang/c/lib/Makefile
src/stores/Makefile
src/stores/dbfs/Makefile
src/apps/Makefile
src/apps/fuse/Makefile
])
AC_OUTPUT
