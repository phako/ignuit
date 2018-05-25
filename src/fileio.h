/* fileio.h
 *
 * Copyright (C) 2008, 2009, 2017 Timothy Richard Musson
 *
 * Email: <trmusson@gmail.com>
 * WWW:   http://homepages.ihug.co.nz/~trmusson/programs.html#ignuit
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef HAVE_FILEIO_H
#define HAVE_FILEIO_H


File*       fileio_load (const gchar *fname, GError **err);

gboolean    fileio_save (Ignuit *ig, const gchar *fname, GError **err);

File*       fileio_import_csv (const gchar *fname,
                gchar delimiter, GError **err);

File*       fileio_import_xml (Ignuit *ig, const gchar *fname,
                const gchar *filter, GError **err);

gboolean    fileio_export_csv (File *f, const gchar *fname,
                gchar delimiter, gboolean excl_markup, GError **err);

gboolean    fileio_export_xml (Ignuit *ig, const gchar *fname,
                const gchar *filter, gboolean excl_markup, GError **err);


#endif /* HAVE_FILEIO_H */

