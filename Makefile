#!/usr/bin/make -f
# Waf Makefile wrapper

all:
	@/home/enrico/projects/sion/trunk/waf build

all-debug:
	@/home/enrico/projects/sion/trunk/waf -v build

all-progress:
	@/home/enrico/projects/sion/trunk/waf -p build

install:
	@if test -n "$(DESTDIR)"; then \
		/home/enrico/projects/sion/trunk/waf install --destdir="$(DESTDIR)"; \
	else \
		/home/enrico/projects/sion/trunk/waf install; \
	fi;

uninstall:
	@if test -n "$(DESTDIR)"; then \
		/home/enrico/projects/sion/trunk/waf uninstall --destdir="$(DESTDIR)"; \
	else \
		/home/enrico/projects/sion/trunk/waf uninstall; \
	fi;

clean:
	@/home/enrico/projects/sion/trunk/waf clean

distclean:
	@/home/enrico/projects/sion/trunk/waf distclean
	@-rm -rf _build_
	@-rm -f Makefile

distcheck:
	@/home/enrico/projects/sion/trunk/waf distcheck

dist:
	@/home/enrico/projects/sion/trunk/waf dist

sign:
	@/home/enrico/projects/sion/trunk/waf --sign

.PHONY: clean dist distclean check uninstall install all

