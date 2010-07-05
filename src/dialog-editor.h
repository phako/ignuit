/* dialog-editor.h
 *
 * Copyright (C) 2008, 2009 Timothy Richard Musson
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


#ifndef HAVE_DIALOG_EDITOR_H
#define HAVE_DIALOG_EDITOR_H


typedef enum {
    ED_TWEAK_NONE     = 1 << 1,
    ED_TWEAK_TITLE    = 1 << 2, /* Window title */
    ED_TWEAK_UI       = 1 << 3, /* Menu and button sensitivity */
    ED_TWEAK_TAG_BAR  = 1 << 4,
    ED_TWEAK_TEXTVIEW = 1 << 5, /* Card front and back textviews */
    ED_TWEAK_INFO     = 1 << 6  /* Info textview and status bar */
} EdTweak;

#define ED_TWEAK_ALL \
    ED_TWEAK_TITLE | ED_TWEAK_UI | ED_TWEAK_TAG_BAR | ED_TWEAK_TEXTVIEW | \
    ED_TWEAK_INFO


void   dialog_editor (Ignuit *ig);
void   dialog_editor_check_changed (void);
void   dialog_editor_kill (void);
void   dialog_editor_tweak (EdTweak tweak);
void   dialog_editor_preferences_changed (void);
void   dialog_editor_card_style_changed (void);


#endif /* HAVE_DIALOG_EDITOR_H */

