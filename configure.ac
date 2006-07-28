AC_PREREQ(2.57)
AC_INIT([spkg], 1.0rc4, [megous@megous.com], spkg)
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
#AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_LN_S

# Checks for pkg-config packages
GLIB_REQUIRED="glib-2.0 >= 2.10.0"
PKG_CHECK_MODULES(SPKG, [$GLIB_REQUIRED])
AC_SUBST(SPKG_CFLAGS)
AC_SUBST(SPKG_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADER([Judy.h], [true], AC_MSG_ERROR([spkg requires Judy: http://judy.sf.net]))
AC_CHECK_HEADER([zlib.h], [true], AC_MSG_ERROR([spkg requires zlib: http://www.zlib.net]))
AC_CHECK_HEADER([popt.h], [true], AC_MSG_ERROR([spkg requires popt library]))

# Check for libs.
AC_CHECK_LIB(Judy, Judy1Next, [true], AC_MSG_ERROR([spkg requires Judy: http://judy.sf.net]))
AC_CHECK_LIB(z, compress, [true], AC_MSG_ERROR([spkg requires zlib: http://www.zlib.net]))
AC_CHECK_LIB(popt, poptGetArg, [true], AC_MSG_ERROR([spkg requires popt library]))

# Debug
#AC_ARG_ENABLE(debug,
#  AS_HELP_STRING([--enable-debug],
#                 [Compile with debugging information.]),
#  [use_debug=$enableval], [use_debug=no])
#
#if test x$use_debug = xno ; then
#  true
#fi

AC_ARG_ENABLE(static-spkg,
  AS_HELP_STRING([--enable-static-spkg],
                 [Create static spkg executable.]),
  [create_static=$enableval], [create_static=no])

AM_CONDITIONAL([STATIC_SPKG],[test "x$create_static" = "xyes"])

# Output
AC_OUTPUT([
	Makefile
	src/Makefile
	include/Makefile
	docs/Makefile
	tests/Makefile
    libspkg.pc
])
