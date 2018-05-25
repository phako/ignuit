/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
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


#include <config.h>
#include <gnome.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "main.h"
#include "file.h"
#include "card.h"


GDate today;


static const gchar s_blank[] = "";


struct _File {

    gchar       *fname;
    gchar       *title;
    gchar       *author;
    gchar       *description;
    gchar       *homepage;
    gchar       *license;
    gchar       *license_uri;
    CardStyle   card_style;

    gboolean    changed;            /* Changed since last save? */

    GList       *categories;        /* List of all categories in file. */
    Category    *current_category;  /* Currently selected category. */
    GList       *current_item;      /* Currently selected card in category. */
    GList       *category_order;    /* Order for saving. */

    GList       *cards;             /* List of all cards in file. */
    GList       *tags;              /* List of all tags in file. */

    Category    *trash;             /* Deleted cards. */
    Category    *search;            /* Search results. */
};


File*
file_new (void)
{
    File *f;

    f = g_new0 (File, 1);
    file_set_title (f, _("New File"));
    file_set_card_style (f, CARD_STYLE_KEYWORDS_AND_CENTERED_TEXT);
    f->search = category_new (NULL);

    return f;
}


void
file_free (File *f, gboolean free_cards)
{
    g_free (f->fname);
    g_free (f->title);
    g_free (f->author);
    g_free (f->description);
    g_free (f->homepage);
    g_free (f->license);
    g_free (f->license_uri);

    if (f->categories != NULL) {
        g_list_foreach (f->categories, (GFunc)category_free,
            GINT_TO_POINTER(free_cards));
        g_list_free (f->categories);
    }

    if (f->category_order != NULL)
        g_list_free (f->category_order);

    if (f->cards != NULL)
        g_list_free (f->cards);

    tag_list_free (f->tags);

    file_clear_search (f, FALSE);
    file_clear_trash (f);

    g_free (f);
}


void
file_set_changed (File *f, gboolean changed)
{
    f->changed = changed;
}


gboolean
file_get_changed (File *f)
{
    return f->changed;
}


void
file_set_filename (File *f, const gchar *fname)
{
    g_free (f->fname);
    f->fname = g_strdup (fname);
}


void
file_set_title (File *f, const gchar *title)
{
    g_free (f->title);
    f->title = g_strdup (title);
}


void
file_set_author (File *f, const gchar *author)
{
    g_free (f->author);
    f->author = g_strdup (author);
}


void
file_set_description (File *f, const gchar *description)
{
    g_free (f->description);
    f->description = g_strdup (description);
}


void
file_set_homepage (File *f, const gchar *uri)
{
    g_free (f->homepage);
    f->homepage = g_strdup (uri);
}


void
file_set_license (File *f, const gchar *license)
{
    g_free (f->license);
    f->license = g_strdup (license);
}


void
file_set_license_uri (File *f, const gchar *uri)
{
    g_free (f->license_uri);
    f->license_uri = g_strdup (uri);
}


const gchar*
file_get_filename (File *f)
{
    return f->fname;
}


const gchar*
file_get_title (File *f)
{
    return f->title ? f->title : s_blank;
}


const gchar*
file_get_author (File *f)
{
    return f->author ? f->author : s_blank;
}


const gchar*
file_get_description (File *f)
{
    return f->description ? f->description : s_blank;
}


const gchar*
file_get_homepage (File *f)
{
    return f->homepage ? f->homepage : s_blank;
}


const gchar*
file_get_license (File *f)
{
    return f->license ? f->license : s_blank;
}


const gchar*
file_get_license_uri (File *f)
{
    return f->license_uri ? f->license_uri : s_blank;
}


void
file_set_card_style (File *f, CardStyle style)
{
    if (style == CARD_STYLE_NONE)
        style = CARD_STYLE_KEYWORDS_AND_CENTERED_TEXT;

    f->card_style = style;
}


CardStyle
file_get_card_style (File *f)
{
    return f->card_style;
}


void
file_set_current_category (File *f, Category *cat)
{
    f->current_category = cat;
}


Category*
file_get_current_category (File *f)
{
    return f->current_category;
}


GList*
file_add_category (File *f, Category *cat)
{
    f->categories = g_list_append (f->categories, cat);
    return g_list_last (f->categories);
}


GList*
file_remove_category (File *f, Category *cat)
{
    GList *cur, *next;

    /* Remove the given category and return the next category. If there is
     * no next category, return the previous category, or NULL. */

    cur = g_list_find (f->categories, cat);
    next = cur->next ? cur->next : cur->prev;

    f->categories = g_list_remove_link (f->categories, cur);
    category_free (cat, TRUE);

    f->cards = NULL;
    for (cur = f->categories; cur != NULL; cur = cur->next) {

        GList *cards, *cur2;

        cards = category_get_cards (CATEGORY(cur));

        for (cur2 = cards; cur2 != NULL; cur2 = cur2->next)
            f->cards = g_list_append (f->cards, cur2->data);

    }

    return next;
}


GList*
file_get_categories (File *f)
{
    return f->categories;
}


gint
file_get_n_categories (File *f)
{
    GList *categories;

    categories = file_get_categories (f);

    if (categories == NULL)
        return 0;

    return g_list_length (categories);
}


gboolean
file_current_category_is_first (File *f)
{
    GList *categories;
    Category *cur;

    if (file_get_n_categories (f) == 0)
        return TRUE;

    cur = file_get_current_category (f);
    categories = file_get_category_order (f);

    g_assert (categories != NULL);

    return categories->data == cur;
}


gboolean
file_current_category_is_last (File *f)
{
    GList *categories;
    Category *cur;

    if (file_get_n_categories (f) == 0)
        return TRUE;

    cur = file_get_current_category (f);
    categories = file_get_category_order (f);

    g_assert (categories != NULL);

    return g_list_last (categories)->data == cur;
}


GList*
file_get_current_category_cards (File *f)
{
    if (f->current_category)
        return category_get_cards (f->current_category);

    return NULL;
}


Category*
file_lookup_category (File *f, const gchar *title)
{
    GList *cur;

    for (cur = file_get_categories (f); cur != NULL; cur = cur->next)
        if (strcmp (category_get_title ((Category*)cur->data), title) == 0)
            return (Category*)cur->data;

    return NULL;
}


void
file_set_category_order (File *f, GList *category_order)
{
    if (f->category_order != NULL)
        g_list_free (f->category_order);

    f->category_order = category_order;
}


GList*
file_get_category_order (File *f)
{
    return f->category_order;
}


void
file_add_loaded_card (File *f, Card *c)
{
    /* Used while loading a file, to get cards in the expected order. */

    category_append_card (file_get_current_category (f), c);
    f->cards = g_list_append (f->cards, c);
}


void
file_add_card (File *f, Category *cat, Card *c)
{
    category_prepend_card (cat, c);
    f->cards = g_list_prepend (f->cards, c);
}


void
file_remove_card (File *f, Card *c)
{
    f->cards = g_list_remove (f->cards, c);
    category_remove_card (card_get_category (c), c);
}


GList*
file_get_cards (File *f)
{
    return f->cards;
}


guint
file_get_n_cards (File *f)
{
    return f->cards ? g_list_length (f->cards) : 0;
}


void
file_set_current_item (File *f, GList *item)
{
    f->current_item = item;
}


GList*
file_incr_current_item (File *f, gboolean wrap)
{
    /* Switch to the next item in the list. If wrap is TRUE, wrap around
     * to the start of the list if necessary. Return the new current item,
     * or NULL if there is none. */

    f->current_item = f->current_item->next;

    if (f->current_item == NULL && wrap)
        f->current_item = file_get_current_category_cards (f);

    return f->current_item;
}


GList*
file_decr_current_item (File *f, gboolean wrap)
{
    /* Switch to the previous item in the list. If wrap is TRUE, wrap around
     * to the end of the list if necessary. Return the new current item,
     * or NULL if there is none. */

    f->current_item = f->current_item->prev;

    if (f->current_item == NULL && wrap)
        f->current_item = g_list_last (file_get_current_category_cards (f));

    return f->current_item;
}


GList*
file_get_current_item (File *f)
{
    return f->current_item;
}


Card*
file_get_current_card (File *f)
{
    if (f->current_item != NULL)
        return CARD(f->current_item);

    return NULL;
}


gboolean
file_current_card_is_blank (File *f)
{
    return card_is_blank (file_get_current_card (f));
}


void
file_reset_statistics (File *f)
{
    g_list_foreach (f->cards, (GFunc)card_reset_statistics, NULL);
}


void
file_check_expired (File *f)
{
    GList *cur;
    Card *c;
    gint hour, cmp;
    gboolean expired;


    /* Find and flag any expired cards in the file. */

    date_today (&today);
    hour = get_current_hour ();

    for (cur = f->cards; cur != NULL; cur = cur->next) {

        c = CARD(cur);

        if (card_get_date_expiry (c) == G_DATE_BAD_JULIAN) {
            card_set_group (c, GROUP_NEW);
            continue;
        }

        cmp = g_date_compare (card_get_gdate_expiry (c), &today);

        expired = cmp < 0 || (cmp == 0 && hour >= card_get_time_expiry (c));

        card_set_expired (c, expired);

    }
}


void
file_clear_search (File *f, gboolean reuse)
{
    if (f->search) {
        category_free (f->search, FALSE);
        f->search = NULL;
    }
    if (reuse) {
        f->search = category_new (NULL);
    }
}


void
file_add_search_card (File *f, Card *c)
{
    category_append_card_without_parent (f->search, c);
}


Category*
file_get_search (File *f)
{
    return f->search;
}


gboolean
file_category_is_search (File *f, Category *cat)
{
    return cat == f->search;
}


void
file_clear_trash (File *f)
{
    if (f->trash != NULL) {
        category_free (f->trash, TRUE);
        f->trash = NULL;
    }
}


void
file_add_trash (File *f, Card *c)
{
    if (f->trash == NULL) {
        f->trash = category_new (_("Trash"));
    }

    category_append_card (f->trash, c);
}


Category*
file_get_trash (File *f)
{
    return f->trash;
}


gboolean
file_category_is_trash (File *f, Category *cat)
{
    return cat == f->trash;
}


CardStyle
file_category_get_card_style (File *f, Category *cat)
{
    if (category_get_card_style (cat) != CARD_STYLE_NONE)
        return category_get_card_style (cat);

    return file_get_card_style (f);
}


CardStyle
file_card_get_card_style (File *f, Card *c)
{
    if (card_get_style (c) != CARD_STYLE_NONE)
        return card_get_style (c);

    return file_category_get_card_style (f, card_get_category (c));
}


GList*
file_get_tags (File *f)
{
    return f->tags;
}


gboolean
file_card_add_new_tags (File *f, Card *c, const gchar *s)
{
    gboolean changed = FALSE;

    /* Give additional tags to a card. */

    f->tags = tag_list_add_new_tags_with_card (c, f->tags, s, &changed);

    return changed;
}


gboolean
file_card_add_new_tags_from_strv (File *f, Card *c, gchar **s)
{
    gboolean changed = FALSE;

    /* Give additional tags to a card. */

    while (*s != NULL) {
        if (*s[0] != '\0')
            f->tags = tag_list_add_new_tag_with_card (c, f->tags, *s, &changed);
        s++;
    }

    return changed;
}


gboolean
file_card_remove_tag (File *f, Card *c, Tag *t)
{
    gboolean changed;

    g_assert (tag_get_ref_count (t) > 1);

    changed = card_remove_tag (c, t);

    if (tag_get_ref_count (t) == 1)
        f->tags = tag_list_remove_tag (f->tags, t, &changed);

    return changed;
}


GList*
file_new_tags_from_string (File *f, const gchar *s, gboolean *changed)
{
    gchar **strv, **name;
    GList *item, *tlist = NULL;
    Tag *t;


    /* Add the named tags to the file if they don't already exist, and
     * return them as a tag list. */

    strv = g_strsplit (s, " ", 0);
    name = strv;

    while (*name != NULL) {
        if (*name[0] != '\0') {
            item = tag_list_lookup_tag_by_name (f->tags, *name);
            if (item != NULL) {
                t = TAG(item);
            }
            else {
                t = tag_new (*name);
                f->tags = tag_list_add_tag (f->tags, t, changed);
            }

            tlist = g_list_insert_sorted (tlist, t, (GCompareFunc)tag_cmp);
        }
        name++;
    }

    return tlist;
}


gboolean
file_card_set_tags_from_string (File *f, Card *c, const gchar *s)
{
    gboolean changed = FALSE;
    GList *tlist;

    /* Replace this card's tags with a different set of tags. */

    tlist = file_new_tags_from_string (f, s, &changed);
    changed |= card_set_tags (c, tlist);

    return changed;
}


void
file_delete_unused_tags (File *f)
{
    gboolean changed = FALSE;
    GList *p, *tlist = NULL;


    /* Tags with a ref_count of 1 are not currently in use by any card. */

    for (p = f->tags; p != NULL; p = p->next)
        if (tag_get_ref_count (TAG(p)) == 1)
            tlist = tag_list_add_tag (tlist, TAG(p), &changed);

    /* Remove those tags from the file */

    for (p = tlist; p != NULL; p = p->next)
        f->tags = tag_list_remove_tag (f->tags, TAG(p), &changed);

    /* And delete them */

    tag_list_free (tlist);
}


gboolean
file_rename_tag (File *f, const gchar *old_name, const gchar *new_name)
{
    gboolean dummy;
    GList *item;
    Tag *t;


    /* Rename a tag and return TRUE. If the name is already in use,
     * do nothing and return FALSE. */

    if (tag_list_lookup_tag_by_name (f->tags, new_name) != NULL)
        return FALSE;

    item = tag_list_lookup_tag_by_name (f->tags, old_name);
    g_assert (item != NULL);
    t = TAG(item);

    /* Remove the tag, rename it, then re-insert it in alphabetical order. */

    f->tags = tag_list_remove_tag (f->tags, t, &dummy);
    tag_set_name (t, new_name);
    f->tags = tag_list_add_tag (f->tags, t, &dummy);

    return TRUE;
}

