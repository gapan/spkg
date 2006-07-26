AUTOMAKE_OPTIONS = foreign
SUBDIRS = src include tests docs

EXTRA_DIST = BENCHMARKS README.cs libspkg.pc.in slack-desc spkg.SlackBuild
CLEANFILES = libspkg.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libspkg.pc

docdir = $(prefix)/doc/spkg-$(VERSION)
doc_DATA = COPYING INSTALL NEWS TODO README BENCHMARKS README.cs
