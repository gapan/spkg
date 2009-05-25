AM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/win32 \
	$(SPKG_CFLAGS) \
	-DSPKG_CONFDIR=\"$(sysconfdir)/spkg\" \
	-D_GNU_SOURCE

# Library build reciepe
libwin32_a_SOURCES = \
	win32.c \
	flock.h \
	flock.c \
	fnmatch.h \
	fnmatch.c \
	getline.c \
	ansidecl.h \
	safe-ctype.h \
	safe-ctype.c

lib_LIBRARIES = libwin32.a
