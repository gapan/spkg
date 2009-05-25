AC_PREREQ(2.60)
AC_INIT([spkg], 1.0rc9, [megous@megous.com], spkg)
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
#AC_DISABLE_STATIC
#AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_RANLIB
AM_PROG_CC_C_O
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_HEADER_STDC

# Checks for pkg-config packages
GLIB_REQUIRED="glib-2.0 >= 2.2.0"
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

# Assume broken pkgdb 
AC_ARG_ENABLE(assume-broken-pkgdb,AC_HELP_STRING([--enable-assume-broken-pkgdb],
[Compile with support for broken package databases that may contain
non-normalized paths with multiple slashes. This doubles time necessary to load
file database. To see if you need to enable this option, check paths in your
package database with 'grep // /var/log/packages/*' command.]))

AM_CONDITIONAL(ENABLE_ASSUME_BROKEN_PKGDB, test "x$enable_assume_broken_pkgdb" = xyes)

# Static spkg
AC_ARG_ENABLE(static-spkg,
  AS_HELP_STRING([--enable-static-spkg],
                 [Create static spkg executable. Use --enable-static-spkg=only for 
                 creating only static spkg binary that will be used by default.]),
                 [create_static=$enableval], [create_static=no])

AM_CONDITIONAL([STATIC_SPKG],[test "x$create_static" = "xyes"])
AM_CONDITIONAL([STATIC_SPKG_ONLY],[test "x$create_static" = "xonly"])

AC_ARG_ENABLE(legacy,
  AS_HELP_STRING([--disable-legacy],
                 [Disable legacy checks, like symlinks in the archive. (EXPERIMENTAL)]),
                 [legacy_checks=$enableval], [legacy_checks=yes])

AM_CONDITIONAL([LEGACY_CHECKS],[test x$legacy_checks = xyes])

# Windows
AM_CONDITIONAL(WINDOWS, test "x$version_type" = xwindows)

# Output
AC_OUTPUT([
    Makefile
    src/Makefile
    src/win32/Makefile
    include/Makefile
    docs/Makefile
    tests/Makefile
    libspkg.pc
])
