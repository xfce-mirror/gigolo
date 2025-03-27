
bin_PROGRAMS = gigolo

gigolo_SOURCES =								\
	main.c main.h								\
	common.c common.h							\
	window.c window.h							\
	bookmark.c bookmark.h						\
	settings.c settings.h						\
	backendgvfs.c backendgvfs.h					\
	menubuttonaction.c menubuttonaction.h		\
	mountdialog.c mountdialog.h					\
	bookmarkpanel.c bookmarkpanel.h				\
	bookmarkdialog.c bookmarkdialog.h			\
	bookmarkeditdialog.c bookmarkeditdialog.h	\
	browsenetworkpanel.c browsenetworkpanel.h	\
	preferencesdialog.c preferencesdialog.h		\
	gigolo_ui.h


AM_CFLAGS = -DLOCALEDIR=\""$(localedir)"\"

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

gigolo_LDADD = \
	$(INTLLIBS) \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

EXTRA_DIST =	\
	meson.build

if MAINTAINER_MODE

BUILT_SOURCES = \
	gigolo_ui.h

gigolo_ui.h: gigolo.ui
	xdt-csource --static --strip-comments --strip-content --name=gigolo_ui $< >$@

endif
