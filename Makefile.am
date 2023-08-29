ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
SUBDIRS = src include docs tests

EXTRA_DIST = BENCHMARKS libspkg.pc.in slack-desc slack-required spkg.SlackBuild
CLEANFILES = libspkg.pc

if !STATIC_SPKG_ONLY
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libspkg.pc
endif

docdir = $(prefix)/doc/spkg-$(VERSION)
doc_DATA = COPYING INSTALL NEWS TODO README.md BENCHMARKS
