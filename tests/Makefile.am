AM_CFLAGS= \
	-I$(top_srcdir)/include \
	$(SPKG_CFLAGS) \
	-D_GNU_SOURCE

noinst_PROGRAMS = \
  test-untgz \
  test-parsers \
  test-path \
  test-mkdir \
  test-blacklist \
  test-rm

LDADD = $(top_builddir)/src/libspkg.a $(SPKG_LIBS) -lz -lJudy

test_blacklist_SOURCES = test-blacklist.c
test_untgz_SOURCES = test-untgz.c
test_parsers_SOURCES = test-parsers.c
test_path_SOURCES = test-path.c
test_mkdir_SOURCES = test-mkdir.c
test_rm_SOURCES = test-rm.c

EXTRA_DIST = test-pkgtools-speed.sh
