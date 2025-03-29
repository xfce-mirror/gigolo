## Process this file with automake to produce Makefile.in

DOCFILES = README.md ChangeLog NEWS COPYING AUTHORS TODO

SUBDIRS =                       \
    icons                       \
    po                          \
    src

distclean-local:
	rm -rf *.spec *.cache *~

distuninstallcheck_listfiles = 						\
	find . -type f -print | grep -v ./share/icons/hicolor/icon-theme.cache


EXTRA_DIST =					\
	autogen.sh					\
	gigolo.desktop.in			\
	src/gigolo.ui				\
	xfce-revision.h.in			\
	meson_options.txt			\
	meson.build


DISTCLEANFILES =				\
	gigolo.1					\
	gigolo.desktop

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

gigolo.desktop: gigolo.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
