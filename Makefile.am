## Process this file with automake to produce Makefile.in

DOCFILES = README ChangeLog NEWS COPYING AUTHORS TODO

SUBDIRS = src po

EXTRA_DIST =					\
	autogen.sh					\
	wscript						\
	waf							\
	README.I18N					\
	sion.desktop.in				\
	intltool-extract.in			\
	intltool-merge.in			\
	intltool-update.in


DISTCLEANFILES =				\
	sion.1						\
	sion.desktop				\
	intltool-extract			\
	intltool-merge				\
	intltool-update

install-data-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	for file in $(DOCFILES); do \
	  $(INSTALL_DATA) $(top_srcdir)/$$file $(DESTDIR)$(docdir)/$$file; \
	done

uninstall-local:
	for file in $(DOCFILES); do \
	  rm $(DESTDIR)$(docdir)/$$file; \
	done


man_MANS=sion.1

desktopdir = $(datadir)/applications
desktop_in_files = sion.desktop
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
