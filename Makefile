#!/usr/bin/make -f
# Waf Makefile wrapper

all:
	@./waf build

all-debug:
	@./waf -v build

all-progress:
	@./waf -p build

install:
	@if test -n "$(DESTDIR)"; then \
		./waf install --destdir="$(DESTDIR)"; \
	else \
		./waf install; \
	fi;

uninstall:
	@if test -n "$(DESTDIR)"; then \
		./waf uninstall --destdir="$(DESTDIR)"; \
	else \
		./waf uninstall; \
	fi;

clean:
	@./waf clean

distclean:
	@./waf distclean
	@-rm -rf _build_
	@-rm -f Makefile

distcheck:
	@./waf distcheck

dist:
	@./waf dist

sign:
	@./waf --sign

.PHONY: clean dist distclean check uninstall install all

