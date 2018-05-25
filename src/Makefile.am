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


AM_CPPFLAGS = \
	$(DISABLE_DEPRECATED) \
	-DPREFIX=\""$(prefix)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = ignuit

ignuit_SOURCES = \
	app-window.h app-window.c                 \
	audio.h audio.c                           \
	card.h card.c                             \
	csvio.h csvio.c                           \
	dialog-about.h dialog-about.c             \
	dialog-editor.h dialog-editor.c           \
	dialog-find.h dialog-find.c               \
	dialog-preferences.h dialog-preferences.c \
	dialog-properties.h dialog-properties.c   \
	dialog-quiz.h dialog-quiz.c               \
	dialog-tagger.h dialog-tagger.c           \
	dialog-category-properties.h              \
	dialog-category-properties.c              \
	file.h file.c                             \
	fileio.h fileio.c                         \
	latex.h latex.c                           \
	main.h main.c                             \
	prefs.h prefs.c                           \
	textview.h textview.c

ignuit_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

