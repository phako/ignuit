/* textview.h
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


#ifndef HAVE_TEXTVIEW_H
#define HAVE_TEXTVIEW_H


typedef enum {
    TV_EDITOR,
    TV_QUIZ,
    TV_INFO
} TVType;


gchar*  remove_card_markup (const gchar *text);

void    textview_set_properties (GtkTextView *textview, Prefs *prefs,
            CardStyle style, TVType tvtype);

void    textbuf_create_tags (GtkTextBuffer *textbuf);

void    textbuf_clear (GtkTextBuffer *textbuf);

void    textbuf_put (GtkTextBuffer *textbuf, const gchar *text);

void    textbuf_place_cursor (GtkTextBuffer *textbuf, GtkTextIter *iter,
            gint where);

void    textview_display_with_markup (Ignuit *ig, GtkTextView *textview,
            const gchar *text, CardStyle style, gint side);

GtkJustification card_style_get_justification (CardStyle style);


#endif /* HAVE_TEXTVIEW_H */

