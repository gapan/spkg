AM_CFLAGS= \
	-I$(top_srcdir)/include \
	$(GLIB_CFLAGS) \
	-D_GNU_SOURCE \
	-Wall -Wno-pointer-sign

# Self-checking tests run by "make check". Each exits non-zero on failure.
TESTS = \
  test-path \
  test-parsers \
  test-blacklist \
  test-mkdir \
  test-rm

# test-untgz is a manual tool (extracts the given archives into the current
# directory), built by "make check" but not run by it.
check_PROGRAMS = $(TESTS) test-untgz

LDADD = $(top_builddir)/src/libspkg.a $(GLIB_LIBS) -lz -lJudy

test_blacklist_SOURCES = test-blacklist.c check.h
test_untgz_SOURCES = test-untgz.c
test_parsers_SOURCES = test-parsers.c check.h
test_path_SOURCES = test-path.c
test_mkdir_SOURCES = test-mkdir.c check.h
test_rm_SOURCES = test-rm.c check.h

EXTRA_DIST = blacklist test-pkgtools-speed.sh
