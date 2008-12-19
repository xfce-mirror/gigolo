## Process this file with automake to produce Makefile.in

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


man_MANS=sion.1

desktopdir = $(datadir)/applications
desktop_in_files = sion.desktop
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
