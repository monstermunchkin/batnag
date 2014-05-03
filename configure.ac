#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([batnag], [0.5], [thomashipp@gmail.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([getopt.h signal.h stdbool.h stdio.h stdlib.h string.h \
		  sys/wait.h unistd.h], [],
AC_MSG_ERROR([could not include header file]))

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_C_INLINE

# Checks for library functions.
AC_FUNC_FORK

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
