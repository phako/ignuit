/* csvio.h - CSV/TSV file input/output
 *
 * Copyright (C) 2009 Timothy Richard Musson
 *
 * Email: Tim Musson <trmusson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef HAVE_CSVIO_H
#define HAVE_CSVIO_H


typedef struct _Csv Csv;

Csv*            csv_open_r (const gchar *fname, gchar delimiter, GError **err);
Csv*            csv_open_w (const gchar *fname, gchar delimiter, GError **err);
void            csv_close (Csv *csv);

const gchar*    csv_get_filename (Csv *csv);

GList*          csv_read_row (Csv *csv, GError **err);

void            csv_row_clear (Csv *csv);
void            csv_row_add_field (Csv *csv, const gchar *text);
void            csv_write_row (Csv *csv);


#endif /* HAVE_CSVIO_H */

