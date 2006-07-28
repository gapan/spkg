AM_CFLAGS= \
	-I$(top_srcdir)/include \
	$(SPKG_CFLAGS) \
	-D_GNU_SOURCE

EXTRA_DIST = \
	cmd-private.h

SPKG_LIB_SRC = \
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

bin_PROGRAMS = spkg

spkg_LDADD = libspkg.la -lpopt
spkg_SOURCES = main.c

libspkg_la_LIBADD = $(SPKG_LIBS) -lJudy -lz
libspkg_la_LDFLAGS = -version-info 1:0:0 -no-undefined -z now
libspkg_la_SOURCES = $(SPKG_LIB_SRC)

if STATIC_SPKG_ONLY
spkg_LDFLAGS = -all-static
noinst_LTLIBRARIES = libspkg.la
else
lib_LTLIBRARIES = libspkg.la
endif

if STATIC_SPKG
bin_PROGRAMS += spkg.static
spkg_static_LDADD = libspkg.la -lpopt
spkg_static_SOURCES = main.c
spkg_static_LDFLAGS = -all-static
endif

install-exec-hook:
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/ipkg$(EXEEXT)
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/rpkg$(EXEEXT)
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/upkg$(EXEEXT)
	ln -snf spkg$(EXEEXT) $(DESTDIR)$(bindir)/lpkg$(EXEEXT)
