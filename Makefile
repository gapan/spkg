#/----------------------------------------------------------------------\#
#| spkg - The Unofficial Slackware Linux Package Manager                |#
#|                                      designed by Ondøej Jirman, 2005 |#
#|----------------------------------------------------------------------|#
#|          No copy/usage restrictions are imposed on anybody.          |#
#\----------------------------------------------------------------------/#
DESTDIR :=
PREFIX := /usr/local
DEBUG := no
ASSERTS := yes
BENCH := yes
VERSION := 20050618

#CC := gcc-3.4.4
CC := gcc
AR := ar
CFLAGS := -pipe -Wall
CPPFLAGS := -Iinclude -Ilibs/sqlite -Ilibs/glib -D_GNU_SOURCE -DSPKG_VERSION=$(VERSION)
LDFLAGS := -lz libs/sqlite/libsqlite3.a libs/glib/libglib-2.0.a
ifeq ($(DEBUG),yes)
CFLAGS +=  -ggdb3 -O0
CPPFLAGS += -D__DEBUG=1
else
CFLAGS += -ggdb1 -O2 -march=i486 -mcpu=i686 -fomit-frame-pointer
endif
ifeq ($(BENCH),yes)
CPPFLAGS += -D__BENCH=1
endif
ifeq ($(ASSERTS),no)
CPPFLAGS += -DG_DISABLE_ASSERT
endif

objs-spkg := main.o pkgtools.o untgz.o sys.o sql.o filedb.o pkgdb.o \
  pkgname.o taction.o

# magic barrier
export MAKEFLAGS += --no-print-directory -r

objs-spkg := $(addprefix .build/, $(objs-spkg))
objs-all := $(sort $(objs-spkg))
dep-files := $(addprefix .build/,$(addsuffix .d,$(basename $(notdir $(objs-all)))))

# default
vpath %.c src

.PHONY: all
all: spkg python-build

include Makefile.tests

spkg: .build/libspkg.a
	$(CC) $^ $(LDFLAGS) -o $@

.build/libspkg.a: $(objs-spkg)
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

.PHONY: python-build python-install
python-build: .build/libspkg.a
	( python setup.py build )

python-install:
ifneq ($(DESTDIR),)
	( python setup.py install --root=$(DESTDIR) )
else
	( python setup.py install )
endif

.PHONY: install uninstall
# installation
install: all docs
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/bin
	install -o root -g bin -m 0755 spkg $(DESTDIR)$(PREFIX)/bin/
	strip $(DESTDIR)$(PREFIX)/bin/spkg
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/man/man1/
	install -o root -g root -m 0644 docs/spkg.1 $(DESTDIR)$(PREFIX)/man/man1/
	gzip -f -9 $(DESTDIR)$(PREFIX)/man/man1/spkg.1
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/doc/spkg-$(VERSION)
	install -d -o root -g root -m 0755 $(DESTDIR)$(PREFIX)/doc/spkg-$(VERSION)/html
	install -o root -g root -m 0644 README INSTALL HACKING NEWS TODO $(DESTDIR)$(PREFIX)/doc/spkg-$(VERSION)
	install -o root -g root -m 0644 docs/html/* $(DESTDIR)$(PREFIX)/doc/spkg-$(VERSION)/html

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/spkg
	rm -f $(DESTDIR)$(PREFIX)/man/man1/spkg.1.gz
	rm -rf $(DESTDIR)$(PREFIX)/doc/spkg-$(VERSION)

.PHONY: slackpkg dist
slackpkg:
	make clean
	rm -rf pkg
	make install PREFIX=/usr STATIC=no DEBUG=no DESTDIR=./pkg
	install -d -o root -g root -m 0755 ./pkg/install
	install -o root -g root -m 0644 docs/slack-desc ./pkg/install/
	( cd pkg ; makepkg -l y -c n ../spkg-$(VERSION)-i486-1.tgz )
	rm -rf pkg

dist: docs
	rm -rf spkg-$(VERSION)
	mkdir -p spkg-$(VERSION)
	tar c `tla inventory -s` | tar xC spkg-$(VERSION)
	tla changelog > spkg-$(VERSION)/ChangeLog
	cp -a docs/html spkg-$(VERSION)/docs
	tar czf spkg-$(VERSION).tar.gz spkg-$(VERSION)
	rm -rf spkg-$(VERSION)

.PHONY: docs web web-base web-files
docs:
	rm -rf docs/html
	doxygen docs/Doxyfile
	rm -f docs/html/doxygen.png

web-base:
	rm -rf .website
	mkdir -p .website
	tar cC docs/web `cd docs/web ; tla inventory -s` | tar xC .website
	sed -i 's/@VER@/$(VERSION)/g ; s/@DATE@/$(shell LANG=C date)/g' .website/*.php .website/inc/*.php
	sed -i 's/@SPKG@/<strong style="color:darkblue;"><span style="color:red;">s<\/span>pkg<\/strong>/g' .website/*.php .website/inc/*.php

web-files: docs dist #slackpkg
	mkdir -p .website/dl/spkg-docs
	cp -r docs/html/* .website/dl/spkg-docs
	( cd .website/dl ; tar czf spkg-docs.tar.gz spkg-docs )
	mv spkg-$(VERSION).tar.gz .website/dl
#	mv spkg-$(VERSION)-i486-1.tgz .website/dl

web: web-base web-files
        
.PHONY: clean mrproper
clean: tests-clean
	-rm -rf .build/*.o .build/*.a spkg build

mrproper: clean
	-rm -rf .build docs/html .website
