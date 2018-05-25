# Copyright (C) 2009, 2012, 2016 Timothy Richard Musson

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


SUBDIRS = src data doc po help

gladedir         = $(datadir)/$(PACKAGE)/glade
glade_DATA       = data/glade/main.glade        \
                   data/glade/properties.glade  \
                   data/glade/preferences.glade \
                   data/glade/find.glade        \
                   data/glade/editor.glade      \
                   data/glade/quiz.glade        \
                   data/glade/tagger.glade      \
                   data/glade/category_properties.glade

importdir        = $(datadir)/$(PACKAGE)/import
import_DATA      = data/import/Pauker.xsl        \
                   data/import/Granule.xsl       \
                   data/import/jMemorize.xsl     \
                   data/import/KVocTrain_1.0.xsl \
                   data/import/Mnemosyne.xsl

exportdir        = $(datadir)/$(PACKAGE)/export
export_DATA      = data/export/Pauker.xsl        \
                   data/export/Granule.xsl       \
                   data/export/jMemorize.xsl     \
                   data/export/Mnemosyne.xsl

examples         = examples/countries_and_capitals-en.xml \
                   examples/numbers_days_months-en_ru.xml \
                   examples/example_latex.xml             \
                   examples/csv_import_test.csv

EXTRA_DIST       = $(glade_DATA)          \
                   $(import_DATA)         \
                   $(export_DATA)         \
                   $(examples)            \
                   gnome-doc-utils.make   \
                   COPYING.extras         \
                   TODO

DISTCLEANFILES   = intltool-extract       \
                   intltool-merge         \
                   intltool-update

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

