AM_CFLAGS= \
	-I$(top_srcdir)/include \
	$(GLIB_CFLAGS) \
	-D_GNU_SOURCE \
	-Wall -Wno-pointer-sign

# Self-checking tests run by "make check". Each exits non-zero on failure.
TESTS = \
  test-path \
  test-parsers \
  test-misc \
  test-archive \
  test-taction \
  test-pkgdb \
  test-interrupt \
  test-blacklist \
  test-mkdir \
  test-rm \
  test-commands.sh

# test-commands.sh runs the spkg binary built in src/.
AM_TESTS_ENVIRONMENT = SPKG=$(top_builddir)/src/spkg; export SPKG;
if LEGACY_CHECKS
AM_TESTS_ENVIRONMENT += SPKG_LEGACY_CHECKS=1; export SPKG_LEGACY_CHECKS;
endif
if SANITIZERS
# fakeroot (and possibly the user's environment) preloads libraries
# before the ASan runtime, which ASan refuses by default.
# "make check ASAN_OPTIONS=detect_leaks=0" turns leak detection off.
AM_TESTS_ENVIRONMENT += ASAN_OPTIONS=verify_asan_link_order=0:$${ASAN_OPTIONS-}; export ASAN_OPTIONS;
AM_TESTS_ENVIRONMENT += UBSAN_OPTIONS=print_stacktrace=1:$${UBSAN_OPTIONS-}; export UBSAN_OPTIONS;
endif

# test-untgz is a manual tool (extracts the given archives into the current
# directory), built by "make check" but not run by it.
check_PROGRAMS = \
  test-path \
  test-parsers \
  test-misc \
  test-archive \
  test-taction \
  test-pkgdb \
  test-interrupt \
  test-blacklist \
  test-mkdir \
  test-rm \
  test-untgz

dist_check_SCRIPTS = test-commands.sh

LDADD = $(top_builddir)/src/libspkg.a $(GLIB_LIBS) -lz -lJudy

test_blacklist_SOURCES = test-blacklist.c check.h
test_untgz_SOURCES = test-untgz.c
test_parsers_SOURCES = test-parsers.c check.h
test_path_SOURCES = test-path.c check.h
test_misc_SOURCES = test-misc.c check.h
test_archive_SOURCES = test-archive.c check.h
test_taction_SOURCES = test-taction.c check.h
test_pkgdb_SOURCES = test-pkgdb.c check.h
test_interrupt_SOURCES = test-interrupt.c check.h
# the test interrupts commands right after these (see test-interrupt.c)
test_interrupt_LDFLAGS = -Wl,--wrap=db_add_pkg -Wl,--wrap=db_replace_pkg
test_interrupt_LDADD = $(LDADD) -lpopt
test_mkdir_SOURCES = test-mkdir.c check.h
test_rm_SOURCES = test-rm.c check.h

EXTRA_DIST = blacklist test-pkgtools-speed.sh
