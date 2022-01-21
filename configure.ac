AC_PREREQ([2.68])
AC_INIT([spkg], [1.4], [vlahavas~at~gmail~dot~com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB(Judy, [Judy1Next], [true], AC_MSG_ERROR([spkg requires Judy: http://judy.sf.net]))
AC_CHECK_LIB(z, [compress], [true], AC_MSG_ERROR([spkg requires zlib: http://www.zlib.net]))
AC_CHECK_LIB(popt, [poptGetArg], [true], AC_MSG_ERROR([spkg requires popt library]))

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/file.h unistd.h utime.h])
AC_CHECK_HEADER([Judy.h], [true], AC_MSG_ERROR([spkg requires Judy: http://judy.sf.net]))
AC_CHECK_HEADER([zlib.h], [true], AC_MSG_ERROR([spkg requires zlib: http://www.zlib.net]))
AC_CHECK_HEADER([popt.h], [true], AC_MSG_ERROR([spkg requires popt library]))

# Checks for pkg-config packages
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.2.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getdelim memset mkdir mkfifo putenv rmdir strchr strerror utime])

# Libtool
LT_INIT([win32-dll])

# Automake
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

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
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/win32/Makefile
    include/Makefile
    docs/Makefile
    tests/Makefile
    libspkg.pc
])
AC_OUTPUT
