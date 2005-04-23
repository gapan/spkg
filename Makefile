#/----------------------------------------------------------------------\#
#| fastpkg                                                              |#
#|----------------------------------------------------------------------|#
#| Slackware Linux Fast Package Management Tools                        |#
#|                               designed by Ondøej (megi) Jirman, 2005 |#
#|----------------------------------------------------------------------|#
#|  No copy/usage restrictions are imposed on anybody using this work.  |#
#\----------------------------------------------------------------------/#
.PHONY: clean mrproper all install install-strip uninstall

DESTDIR :=
PREFIX := /usr/local
DEBUG := no
STATIC := yes
VERSION := 0.1

CC := gcc

LDFLAGS := `pkg-config --libs glib-2.0` -lz
CPPFLAGS := -D_GNU_SOURCE -I. `pkg-config --cflags glib-2.0` `pkg-config --cflags sqlite3`
CFLAGS := -pipe -Wall -Werror
ifeq ($(DEBUG),yes)
CFLAGS +=  -ggdb3 -O0
CPPFLAGS += -DFPKG_DEBUG 
else
CFLAGS += -g0 -O2 -march=i486 -mcpu=i686 -fomit-frame-pointer
endif
ifeq ($(STATIC),yes)
LDFLAGS += `pkg-config --variable=libdir sqlite3`/libsqlite3.a
else
LDFLAGS += `pkg-config --libs sqlite3`
endif

objs-fastpkg := main.o pkgtools.o untgz.o sysutils.o # pkgdb.o

# magic barrier

export MAKEFLAGS += --no-print-directory -r
CLEANFILES := .o fastpkg

objs-fastpkg := $(addprefix .o/, $(objs-fastpkg))
objs-all := $(sort $(objs-fastpkg))
dep-files := $(addprefix .dep/,$(addsuffix .d,$(basename $(notdir $(objs-all)))))

# default
all: fastpkg

fastpkg: $(objs-fastpkg)
	$(CC) $^ $(LDFLAGS) -o $@

.o/%.o: %.c
	@mkdir -p .o
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# installation
install-strip: install
	strip $(DESTDIR)$(PREFIX)/bin/fastpkg

install: all
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)
	install -o root -g bin -m 0755 fastpkg $(DESTDIR)$(PREFIX)/bin/
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/man/man1/
	install -o root -g root -m 0644 fastpkg.1 $(DESTDIR)$(PREFIX)/man/man1/
	gzip -9 $(DESTDIR)$(PREFIX)/man/man1/fastpkg.1
	install -o root -g root -m 0644 README INSTALL HACKING NEWS TODO $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/fastpkg
	rm -f $(DESTDIR)$(PREFIX)/man/man1/fastpkg.1
	rm -rf $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)

slackpkg:
	make clean
	rm -rf pkg
	make install-strip PREFIX=/usr STATIC=yes DEBUG=no DESTDIR=./pkg
	install -d -o root -g root -m 0755 ./pkg/install
	install -o root -g root -m 0644 slack-desc ./pkg/install/
	( cd pkg ; makepkg -l y -c n ../fastpkg-$(VERSION)-i486-1.tgz )
	rm -rf pkg
	make mrproper

# generate deps
vpath %.c .
.dep/%.d: %.c
	@echo "DEP    $<"
	@mkdir -p .dep
	@$(CC) -MM -MG -MP -MF $@ -MT ".o/$(<F:.c=.o) $@" $(CPPFLAGS) $<
ifneq ($(dep-files),)
-include $(dep-files)
endif

# cleansing
clean:
	-rm -rf $(CLEANFILES)

mrproper:
	-rm -rf $(CLEANFILES) .dep

