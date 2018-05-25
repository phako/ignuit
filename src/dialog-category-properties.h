/* dialog-category-properties.h
 *
 * Copyright (C) 2016 Timothy Richard Musson
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


#ifndef HAVE_DIALOG_CATEGORY_PROPERTIES_H
#define HAVE_DIALOG_CATEGORY_PROPERTIES_H

void   dialog_category_properties (Ignuit *ig, GtkWidget *m_checkbox_fixed_order);
void   dialog_category_properties_tweak (void);
void   dialog_category_properties_check_changed (void);
void   dialog_category_properties_close (void);

#endif /* HAVE_DIALOG_CATEGORY_PROPERTIES_H */

