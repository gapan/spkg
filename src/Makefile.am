AM_CFLAGS= \
	-I$(top_srcdir)/include \
	$(SPKG_CFLAGS) \
	-D_GNU_SOURCE

lib_LTLIBRARIES = libspkg.la

libspkg_la_LIBADD = \
	$(SPKG_LIBS) -lJudy -lz

libspkg_la_SOURCES = \
	misc.c \
	error.c \
	sys.c \
	path.c \
	untgz.c \
	pkgdb.c \
	taction.c \
	sigtrap.c \
	message.c \
	cmd-install.c \
	cmd-remove.c \
	cmd-upgrade.c \
	cmd-list.c

libspkg_la_LDFLAGS = -version-info 1:0:0 -no-undefined

bin_PROGRAMS = spkg

spkg_LDADD = libspkg.la -lpopt

spkg_SOURCES = \
	main.c

EXTRA_DIST = \
	cmd-private.h

install-exec-hook:
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/ipkg$(EXEEXT)
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/rpkg$(EXEEXT)
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/upkg$(EXEEXT)
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/lpkg$(EXEEXT)

if STATIC_SPKG
bin_PROGRAMS += spkg.static
spkg_static_LDADD = libspkg.la -lpopt
spkg_static_LDFLAGS = -all-static
spkg_static_SOURCES = \
	main.c
endif
