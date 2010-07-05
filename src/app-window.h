/* app-window.h
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


#ifndef HAVE_APP_WINDOW_H
#define HAVE_APP_WINDOW_H


#define COLUMN_DATA  0


enum {
    COLUMN_CATEGORY_DATA = 0,
    COLUMN_CATEGORY_TITLE,
    COLUMN_CATEGORY_CARD_COUNT,
    CATEGORIES_N_COLUMNS
};


enum {
    COLUMN_CARD_DATA = 0,
    COLUMN_CARD_FRONT,
    COLUMN_CARD_BACK,
    COLUMN_CARD_CATEGORY,
    COLUMN_CARD_GROUP,
    COLUMN_CARD_TESTED,
    COLUMN_CARD_EXPIRED,
    COLUMN_CARD_EXPIRY_DATE,
    COLUMN_CARD_EXPIRY_TIME,
    COLUMN_CARD_FLAGGED,
    CARDS_N_COLUMNS
};


void        error_dialog (GtkWindow *parent, const gchar *message, GError *err);

GList*      treev_get_selected_items (GtkTreeView *treev);

void        app_window (Ignuit *ig);

void        app_window_update_appbar (Ignuit *ig);

void        app_window_update_title (Ignuit *ig);

void        app_window_update_expiry_color (Ignuit *ig, GdkColor *color,
                gboolean redraw);

void        app_window_refresh_card_pane (Ignuit *ig, GList *cards);

GList*      app_window_get_category_list (Ignuit *ig);

GList*      app_window_find_treev_iter_with_data (GtkTreeView *treev,
                GtkTreeIter *iter, gpointer data);

void        app_window_refresh_category_row (Ignuit *ig, Category *cat);

void        app_window_refresh_card_row (Ignuit *ig, GList *item);

void        app_window_add_card_iter (Ignuit *ig, GtkTreeIter *iter,
                GList *item);

gboolean    app_window_select_card (Ignuit *ig, GList *item);

gboolean    app_window_select_category (Ignuit *ig, GList *item);

void        app_window_make_trash (Ignuit *ig);

gboolean    ask_yes_or_no (GtkWindow *parent, const gchar *question,
                GtkResponseType def);


#endif /* HAVE_APP_WINDOW_H */

