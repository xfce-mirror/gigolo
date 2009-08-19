dnl Process this file with autoconf to produce a configure script.
dnl $Id$

AC_INIT([gigolo], [0.3.2], [http://bugs.xfce.org/])
AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER(config.h)

AC_GNU_SOURCE
AC_AIX
AC_MINIX

AC_PROG_CC

AC_PROG_INSTALL
AC_PROG_LN_S
IT_PROG_INTLTOOL


# GTK checks
gtk_modules="gtk+-2.0 >= 2.12.0 gio-2.0 >= 2.16.0"
PKG_CHECK_MODULES(GTK, [$gtk_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# --disable-deprecated switch for GTK2 purification
AC_ARG_ENABLE(deprecated, [  --disable-deprecated    Disable deprecated GTK functions. ],
			  [GTK_CFLAGS="$GTK_CFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"], [])

GTK_VERSION=`$PKG_CONFIG --modversion gtk+-2.0`

# i18n
GETTEXT_PACKAGE=gigolo
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package.])

ALL_LINGUAS="`ls po/*.po | awk 'BEGIN { FS="[./]"; ORS=" " } { print $2 }'`"

AM_GLIB_GNU_GETTEXT
# workaround for intltool bug (http://bugzilla.gnome.org/show_bug.cgi?id=490845)
if test "x$MSGFMT" = "xno"; then
	AC_MSG_ERROR([msgfmt not found. Please install the gettext package.])
fi

# intltool hack to define install_sh on Debian/Ubuntu systems
if test "x$install_sh" = "x"; then
	install_sh="`pwd`/install-sh"
	AC_SUBST(install_sh)
fi


AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
gigolo.1
])

echo "----------------------------------------"
echo "Install in                         : ${prefix}"
echo "Using GTK version                  : ${GTK_VERSION}"
echo ""
echo "Configuration is done OK."
echo ""
