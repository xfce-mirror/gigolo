dnl Process this file with autoconf to produce a configure script.
XDT_VERSION_INIT([0.6.0])
m4_define([copyright_year], [2025])

AC_COPYRIGHT([Copyright (c) 2008-copyright_year() The Xfce development team. All rights reserved.])
AC_INIT([gigolo], [xdt_version], [https://gitlab.xfce.org/apps/gigolo])
AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar no-dist-gzip foreign])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIRS([m4])
AM_MAINTAINER_MODE()
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PREREQ([2.69])
AC_REVISION([xdt_version_build])
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

# GTK checks
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.66.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.24.0])

# --disable-deprecated switch for GTK2 purification
AC_ARG_ENABLE(deprecated, [  --disable-deprecated    Disable deprecated GTK functions. ],
			  [GTK_CFLAGS="$GTK_CFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"], [])

GTK_VERSION=`$PKG_CONFIG --modversion gtk+-3.0`

# i18n
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])
AC_SEARCH_LIBS([bind_textdomain_codeset], [intl],
               [AC_DEFINE([HAVE_BIND_TEXTDOMAIN_CODESET], [1], [Define to 1 if you have the 'bind_textdomain_codeset' function.])],
               [])

XDT_FEATURE_DEBUG([xdt_debug_default])

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
gigolo.1
icons/Makefile
icons/16x16/Makefile
icons/24x24/Makefile
icons/32x32/Makefile
icons/48x48/Makefile
icons/64x64/Makefile
icons/96x96/Makefile
icons/128x128/Makefile
icons/scalable/Makefile
])
AC_OUTPUT

echo "----------------------------------------"
echo "Install in                         : ${prefix}"
echo "Using GTK version                  : ${GTK_VERSION}"
echo ""
echo "Configuration is done OK."
echo ""
