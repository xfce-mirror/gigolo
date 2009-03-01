
bin_PROGRAMS = gigolo

gigolo_SOURCES = 								\
	main.c main.h								\
	compat.c compat.h							\
	common.c common.h							\
	window.c window.h							\
	bookmark.c bookmark.h						\
	settings.c settings.h						\
	backendgvfs.c backendgvfs.h					\
	menubuttonaction.c menubuttonaction.h		\
	mountdialog.c mountdialog.h					\
	mountoperation.c mountoperation.h			\
	bookmarkdialog.c bookmarkdialog.h			\
	bookmarkeditdialog.c bookmarkeditdialog.h	\
	preferencesdialog.c preferencesdialog.h


AM_CFLAGS = -DLOCALEDIR=\""$(localedir)"\"


INCLUDES = @GTK_CFLAGS@

gigolo_LDADD = @GTK_LIBS@ $(INTLLIBS)
