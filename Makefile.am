## Process this file with automake to produce Makefile.in

DOCFILES = README.md ChangeLog NEWS COPYING AUTHORS TODO

SUBDIRS =                       \
    icons                       \
    po                          \
    src

EXTRA_DIST =					\
	autogen.sh					\
	gigolo.desktop.in			\
	intltool-extract.in			\
	intltool-merge.in			\
	intltool-update.in


DISTCLEANFILES =				\
	gigolo.1					\
	gigolo.desktop				\
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


man_MANS=gigolo.1

desktopdir = $(datadir)/applications
desktop_in_files = gigolo.desktop
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
