AM_CFLAGS = \
	-I$(top_srcdir)/include \
	$(GLIB_CFLAGS) \
	-DSPKG_CONFDIR=\"$(sysconfdir)/spkg\" \
	-D_GNU_SOURCE \
	-Wall -Wno-pointer-sign

if ENABLE_ASSUME_BROKEN_PKGDB
AM_CFLAGS += -DASSUME_BROKEN_PKGDB=1
endif

if LEGACY_CHECKS
AM_CFLAGS += -DLEGACY_CHECKS=1
endif

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
	cmd-common.c \
	cmd-install.c \
	cmd-remove.c \
	cmd-upgrade.c \
	cmd-list.c

# Library build reciepe
libspkg_a_SOURCES = $(SPKG_LIB_SRC)
spkg_SOURCES = main.c
spkg_LDADD = libspkg.a -lpopt $(GLIB_LIBS) -lJudy -lz

# Build only static spkg as spkg
if STATIC_SPKG_ONLY

bin_PROGRAMS = spkg
noinst_LIBRARIES = libspkg.a
spkg_LDFLAGS = -all-static

else

# Build spkg.static and dynamic spkg binaries
if STATIC_SPKG

bin_PROGRAMS = spkg spkg.static
lib_LIBRARIES = libspkg.a

spkg_static_LDADD = $(spkg_LDADD)
spkg_static_SOURCES = main.c
spkg_static_LDFLAGS = -all-static

else
# Build dynamic spkg binary

bin_PROGRAMS = spkg
lib_LIBRARIES = libspkg.a

endif

endif
