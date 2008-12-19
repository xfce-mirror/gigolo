
bin_PROGRAMS = sion

sion_SOURCES = 									\
	main.c main.h								\
	compat.c compat.h							\
	common.c common.h							\
	window.c window.h							\
	bookmark.c bookmark.h						\
	settings.c settings.h						\
	backendgvfs.c backendgvfs.h					\
	menubuttonaction.c menubuttonaction.h		\
	passworddialog.c passworddialog.h			\
	bookmarkdialog.c bookmarkdialog.h			\
	bookmarkeditdialog.c bookmarkeditdialog.h	\
	preferencesdialog.c preferencesdialog.h


AM_CFLAGS = -DLOCALEDIR=\""$(localedir)"\"


INCLUDES = @GTK_CFLAGS@

sion_LDADD = @GTK_LIBS@ $(INTLLIBS)
