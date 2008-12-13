#!/usr/bin/make -f
# Waf Makefile wrapper
WAF_HOME=/home/enrico/projects/sion

all:
	@/home/enrico/projects/sion/waf build

all-debug:
	@/home/enrico/projects/sion/waf -v build

all-progress:
	@/home/enrico/projects/sion/waf -p build

install:
	if test -n "$(DESTDIR)"; then \
		/home/enrico/projects/sion/waf install --yes --destdir="$(DESTDIR)" --prefix="/usr/local"; \
	else \
		/home/enrico/projects/sion/waf install --yes --prefix="/usr/local"; \
	fi;

uninstall:
	@if test -n "$(DESTDIR)"; then \
		/home/enrico/projects/sion/waf uninstall --destdir="$(DESTDIR)" --prefix="/usr/local"; \
	else \
		/home/enrico/projects/sion/waf uninstall --prefix="/usr/local"; \
	fi;

clean:
	@/home/enrico/projects/sion/waf clean

distclean:
	@/home/enrico/projects/sion/waf distclean
	@-rm -rf _build_
	@-rm -f Makefile

distcheck:
	@/home/enrico/projects/sion/waf distcheck

dist:
	@/home/enrico/projects/sion/waf dist

sign:
	@/home/enrico/projects/sion/waf --sign

.PHONY: clean dist distclean check uninstall install all

