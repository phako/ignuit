/* file.h
 *
 * Copyright (C) 2008, 2009, 2015 Timothy Richard Musson
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


#ifndef HAVE_FILE_H
#define HAVE_FILE_H


#include "card.h"


typedef enum {
    CARD_STYLE_NONE = 0,
    CARD_STYLE_KEYWORDS_AND_CENTERED_TEXT,
    CARD_STYLE_KEYWORDS_AND_SENTENCES,
    CARD_STYLE_SENTENCES
} CardStyle;


typedef struct _File File;

#define file_current_category_is_search(f) \
        file_category_is_search (f, file_get_current_category (f))

#define file_current_category_is_trash(f) \
        file_category_is_trash (f, file_get_current_category (f))

#define file_current_category_is_special(f) \
        (file_category_is_trash (f, file_get_current_category (f)) || \
        file_category_is_search (f, file_get_current_category (f)))


File*           file_new (void);
void            file_free (File *f, gboolean free_cards);

void            file_set_changed (File *f, gboolean changed);
gboolean        file_get_changed (File *f);

void            file_set_filename (File *f, const gchar *fname);
void            file_set_title (File *f, const gchar *title);
void            file_set_author (File *f, const gchar *author);
void            file_set_description (File *f, const gchar *description);
void            file_set_homepage (File *f, const gchar *url);
void            file_set_license (File *f, const gchar *license);
void            file_set_license_uri (File *f, const gchar *uri);

const gchar*    file_get_filename (File *f);
const gchar*    file_get_title (File *f);
const gchar*    file_get_author (File *f);
const gchar*    file_get_description (File *f);
const gchar*    file_get_homepage (File *f);
const gchar*    file_get_license (File *f);
const gchar*    file_get_license_uri (File *f);

void            file_set_card_style (File *f, CardStyle style);
CardStyle       file_get_card_style (File *f);

void            file_set_current_category (File *f, Category *cat);
Category*       file_get_current_category (File *f);

GList*          file_add_category (File *f, Category *cat);
GList*          file_remove_category (File *f, Category *cat);
GList*          file_get_categories (File *f);
GList*          file_get_current_category_cards (File *f);

#if 1
gint            file_get_n_categories (File *f);
gboolean        file_current_category_is_first (File *f);
gboolean        file_current_category_is_last (File *f);
#endif

Category*       file_lookup_category (File *f, const gchar *title);

void            file_set_category_order (File *f, GList *category_order);
GList*          file_get_category_order (File *f);

void            file_add_loaded_card (File *f, Card *c);
void            file_add_card (File *f, Category *cat, Card *c);
void            file_remove_card (File *f, Card *c);
GList*          file_get_cards (File *f);
guint           file_get_n_cards (File *f);

void            file_set_current_item (File *f, GList *item);
GList*          file_incr_current_item (File *f, gboolean wrap);
GList*          file_decr_current_item (File *f, gboolean wrap);
GList*          file_get_current_item (File *f);
Card*           file_get_current_card (File *f);

gboolean        file_current_card_is_blank (File *f);

void            file_reset_statistics (File *f);

void            file_check_expired (File *f);

void            file_clear_search (File *f, gboolean reuse);
void            file_add_search_card (File *f, Card *c);
Category*       file_get_search (File *f);
gboolean        file_category_is_search (File *f, Category *cat);

void            file_clear_trash (File *f);
void            file_add_trash (File *f, Card *c);
Category*       file_get_trash (File *f);
gboolean        file_category_is_trash (File *f, Category *cat);

CardStyle       file_category_get_card_style (File *f, Category *cat);
CardStyle       file_card_get_card_style (File *f, Card *c);

GList*          file_get_tags (File *f);
gboolean        file_card_add_new_tags (File *f, Card *c, const gchar *s);
gboolean        file_card_add_new_tag (File *f, Card *c, const char *name);
gboolean        file_card_add_new_tags_from_strv (File *f, Card *c,
                    gchar **s);
gboolean        file_card_remove_tag (File *f, Card *c, Tag *t);
gboolean        file_card_set_tags_from_string (File *f, Card *c,
                    const gchar *s);
void            file_delete_unused_tags (File *f);
gboolean        file_rename_tag (File *f, const gchar *old_name,
                    const gchar *new_name);


#endif  /* HAVE_FILE_H */

