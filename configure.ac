dnl  Copyright (C) 2008, 2009, 2012, 2015, 2016 Timothy Richard Musson
dnl
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 2, or (at your option)
dnl  any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
dnl  02111-1307, USA.


AC_INIT(ignuit, [2.24.3])
AM_INIT_AUTOMAKE

AC_PREREQ(2.52)

AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([src])

dnl GNOME_COMMON_INIT
dnl GNOME_MAINTAINER_MODE_DEFINES
dnl GNOME_COMPILE_WARNINGS

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.50.0])

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package.])

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

PKG_PROG_PKG_CONFIG
GNOME_DOC_INIT

dnl ************************************

LIBGNOMEUI_REQUIRED=2.24.5
LIBGLADE_REQUIRED=2.6.4
LIBXSLT_REQUIRED=1.1.28
GCONF_REQUIRED=3.2.6
GSTREAMER_REQUIRED=1.8.2

dnl ************************************

PKG_CHECK_MODULES(PACKAGE, \
    libgnomeui-2.0 >= $LIBGNOMEUI_REQUIRED  \
    libglade-2.0 >= $LIBGLADE_REQUIRED      \
    libxslt >= $LIBXSLT_REQUIRED            \
    gconf-2.0 >= $GCONF_REQUIRED            \
    gstreamer-1.0 >= $GSTREAMER_REQUIRED)

AM_GCONF_SOURCE_2
AC_PATH_PROG(GCONFTOOL, gconftool-2)

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/ignuit.desktop.in
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/actions/Makefile
data/icons/22x22/Makefile
data/icons/22x22/actions/Makefile
data/icons/24x24/Makefile
data/icons/24x24/actions/Makefile
data/icons/32x32/Makefile
data/icons/32x32/actions/Makefile
data/icons/48x48/Makefile
data/icons/48x48/apps/Makefile
data/icons/scalable/Makefile
data/icons/scalable/actions/Makefile
data/icons/scalable/apps/Makefile
doc/Makefile
po/Makefile.in
help/Makefile
])

