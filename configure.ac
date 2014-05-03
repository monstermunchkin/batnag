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

# Add preprocessor symbols.
AS_IF([test "x$BATSTAT" = "x"],
      AC_DEFINE([BATSTAT], ["/sys/class/power_supply/BAT0/status"],
		[Battery status]),
      AC_DEFINE_UNQUOTED([BATSTAT], ["$BATSTAT"], [Battery status]))
AS_IF([test "x$BATCAP" = "x"],
      AC_DEFINE([BATCAP], ["/sys/class/power_supply/BAT0/capacity"],
		[Battery capacity]),
      AC_DEFINE_UNQUOTED([BATCAP], ["$BATCAP"], [Battery capacity]))
AC_ARG_VAR(BATSTAT, [Path to battery status (syspath)])
AC_ARG_VAR(BATCAP, [Path to battery capacity (syspath)])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
