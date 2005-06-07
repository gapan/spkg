#/----------------------------------------------------------------------\#
#| fastpkg                                                              |#
#|----------------------------------------------------------------------|#
#| Slackware Linux Fast Package Management Tools                        |#
#|                               designed by Ondøej (megi) Jirman, 2005 |#
#|----------------------------------------------------------------------|#
#|  No copy/usage restrictions are imposed on anybody using this work.  |#
#\----------------------------------------------------------------------/#
DESTDIR :=
PREFIX := /usr/local
DEBUG := no
STATIC := no
VERSION := 0.9.1

CC := gcc
AR := ar
CPPFLAGS := -Iinclude -D_GNU_SOURCE -DFASTPKG_VERSION='"$(VERSION)"' \
  $(shell pkg-config --cflags glib-2.0 sqlite3)
CFLAGS := -pipe -Wall
LDFLAGS := -lz $(shell pkg-config --libs glib-2.0 sqlite3)
ifeq ($(DEBUG),yes)
CFLAGS +=  -ggdb3 -O0
CPPFLAGS += -D__DEBUG=1
else
CFLAGS += -ggdb1 -O2 -march=i486 -mcpu=i686 -fomit-frame-pointer
endif
ifeq ($(STATIC),yes)
LDFLAGS += -static
endif

objs-fastpkg := main.o pkgtools.o untgz.o sys.o sql.o filedb.o pkgdb.o \
  pkgname.o taction.o

# magic barrier
.PHONY: clean mrproper all install install-strip uninstall slackpkg docs
export MAKEFLAGS += --no-print-directory -r

objs-fastpkg := $(addprefix .build/, $(objs-fastpkg))
objs-all := $(sort $(objs-fastpkg))
dep-files := $(addprefix .build/,$(addsuffix .d,$(basename $(notdir $(objs-all)))))

# default
vpath %.c src

all: fastpkg

fastpkg: .build/libfastpkg.a
	$(CC) $^ $(LDFLAGS) -o $@

.build/libfastpkg.a: $(objs-fastpkg)
	$(AR) rcs $@ $^

.build/%.o: %.c
	@mkdir -p .build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# generate deps
.build/%.d: %.c
	@mkdir -p .build
	$(CC) -MM -MG -MP -MF $@ -MT ".build/$(<F:.c=.o) $@" $(CPPFLAGS) $<
ifneq ($(dep-files),)
-include $(dep-files)
endif

# installation
install: all docs
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/bin
	install -o root -g bin -m 0755 fastpkg $(DESTDIR)$(PREFIX)/bin/
	strip $(DESTDIR)$(PREFIX)/bin/fastpkg
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/man/man1/
	install -o root -g root -m 0644 docs/fastpkg.1 $(DESTDIR)$(PREFIX)/man/man1/
	gzip -9 $(DESTDIR)$(PREFIX)/man/man1/fastpkg.1
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)/html
	install -o root -g root -m 0644 README INSTALL HACKING NEWS TODO $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)
	install -o root -g root -m 0644 docs/html/* $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)/html

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/fastpkg
	rm -f $(DESTDIR)$(PREFIX)/man/man1/fastpkg.1
	rm -rf $(DESTDIR)$(PREFIX)/doc/fastpkg-$(VERSION)

slackpkg:
	make clean
	rm -rf pkg
	make install PREFIX=/usr STATIC=no DEBUG=no DESTDIR=./pkg
	install -d -o root -g root -m 0755 ./pkg/install
	install -o root -g root -m 0644 docs/slack-desc ./pkg/install/
	( cd pkg ; makepkg -l y -c n ../fastpkg-$(VERSION)-i486-1.tgz )
	rm -rf pkg

dist: docs
	rm -rf fastpkg-$(VERSION)
	mkdir -p fastpkg-$(VERSION)
	tar c `tla inventory -s` | tar xC fastpkg-$(VERSION)
	tla changelog > fastpkg-$(VERSION)/ChangeLog
	cp -a docs/html fastpkg-$(VERSION)/docs
	tar czf fastpkg-$(VERSION).tar.gz fastpkg-$(VERSION)
	rm -rf fastpkg-$(VERSION)

docs:
	rm -rf docs/html
	doxygen docs/Doxyfile

clean:
	-rm -rf .build/*.o .build/*.a fastpkg

mrproper:
	-rm -rf .build fastpkg docs/html
