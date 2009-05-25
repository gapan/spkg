AUTOMAKE_OPTIONS = foreign
SUBDIRS = src include docs

if !WINDOWS
SUBDIRS += tests
endif

EXTRA_DIST = BENCHMARKS README.cs libspkg.pc.in slack-desc slack-required spkg.SlackBuild
CLEANFILES = libspkg.pc

if !STATIC_SPKG_ONLY
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libspkg.pc
endif

docdir = $(prefix)/doc/spkg-$(VERSION)
doc_DATA = COPYING INSTALL NEWS TODO README BENCHMARKS README.cs
