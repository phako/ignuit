# Copyright (C) 2009, 2015 Timothy Richard Musson

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


SUBDIRS = icons

install-data-local: install-schemas update-icon-cache
uninstall-local: uninstall-schemas update-icon-cache


#
# Manual page
#

#man_MANS=ignuit.1


#
# Desktop file
#

@INTLTOOL_DESKTOP_RULE@

desktopdir          = $(datadir)/applications
desktop_in_in_files = ignuit.desktop.in.in
desktop_in_files    = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA        = $(desktop_in_files:.desktop.in=.desktop)


#
# AppData
#

appdatadir          = $(datadir)/appdata
appdata_DATA        = ignuit.appdata.xml


#
# GConf schema
#

@INTLTOOL_SCHEMAS_RULE@

schemadir           = $(GCONF_SCHEMA_FILE_DIR)
schema_in           = ignuit.schemas.in
schema_DATA         = $(schema_in:.schemas.in=.schemas)

install-schemas: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)"; then \
		for p in $(schema_DATA); do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			gconftool-2 --makefile-install-rule $(top_builddir)/data/$$p; \
		done; \
	fi
endif

uninstall-schemas: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)"; then \
		for p in $(schema_DATA); do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			gconftool-2 --makefile-uninstall-rule $(top_builddir)/data/$$p; \
		done; \
	fi
endif


#
# GTK icon cache
#

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache

update-icon-cache:
	if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi


#
# Extra files to be included in the tarball
# 

EXTRA_DIST          = $(desktop_in_files) \
                      $(schema_in) \
                      $(appdata_DATA)


#
# Extra files to clean up
#

DISTCLEANFILES      = $(desktop_in_files) \
                      $(desktop_DATA)     \
                      $(schema_DATA)


