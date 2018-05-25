/* card.h
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


#ifndef HAVE_CARD_H
#define HAVE_CARD_H


typedef enum {
    GROUP_NEW = 0,  /* Untested cards. */
    GROUP_1,        /* Least well known cards. */
    GROUP_2,
    GROUP_3,
    GROUP_4,
    GROUP_5,
    GROUP_6,
    GROUP_7,
    GROUP_8,        /* Best known cards. */
    N_GROUPS
} Group;

#define GROUP_MAX (N_GROUPS - 1)


typedef enum {
    DATE_CREATED = 0,
    DATE_TESTED,
    DATE_EXPIRY,
    N_CARD_DATES
} CardDate;


#define card_set_date_created(card, date)  card_set_date(card, DATE_CREATED, date)
#define card_get_gdate_created(card)  card_get_gdate(card, DATE_CREATED)
#define card_get_date_created(card)   card_get_date(card, DATE_CREATED)

#define card_set_date_tested(card, date)  card_set_date(card, DATE_TESTED, date)
#define card_get_gdate_tested(card)  card_get_gdate(card, DATE_TESTED)
#define card_get_date_tested(card)   card_get_date(card, DATE_TESTED)

#define card_set_date_expiry(card, date)  card_set_date(card, DATE_EXPIRY, date)
#define card_get_gdate_expiry(card)  card_get_gdate(card, DATE_EXPIRY)
#define card_get_date_expiry(card)   card_get_date(card, DATE_EXPIRY)

#define card_front_is_blank(card) (card_get_front (card)[0] == '\0')
#define card_back_is_blank(card)  (card_get_back (card)[0] == '\0')

#define TAG(glist_item)      ((Tag*)(glist_item)->data)
#define CARD(glist_item)     ((Card*)(glist_item)->data)
#define CATEGORY(glist_item) ((Category*)(glist_item)->data)


typedef struct _Tag Tag;
typedef struct _Card Card;
typedef struct _Category Category;


Card*            card_new ();
Card*            card_copy (Card *src);
void             card_free (Card *c);
void             card_list_free (GList *list, gboolean free_cards);

void             card_set_style (Card *c, gint style);
gint             card_get_style (Card *c);

void             card_set_front (Card *c, const char *text);
void             card_set_back (Card *c, const char *text);
const gchar*     card_get_front (Card *c);
const gchar*     card_get_front_without_markup (Card *c);
const gchar*     card_get_back (Card *c);
const gchar*     card_get_back_without_markup (Card *c);

GList*           card_get_tags (Card *c);
gchar*           card_get_tags_as_string (Card *c);
gboolean         card_add_tag (Card *c, Tag *t);
gboolean         card_set_tags (Card *c, GList *tlist);
gboolean         card_remove_tag (Card *c, Tag *t);

gboolean         card_is_blank (Card *c);

void             card_switch_sides (Card *c);

void             card_set_group (Card *c, Group g);
Group            card_get_group (Card *c);

void             card_set_category (Card *c, Category *cat);
Category*        card_get_category (Card *c);

void             card_set_date (Card *c, CardDate cd, guint32 date);
GDate*           card_get_gdate (Card *c, CardDate cd);
guint32          card_get_date (Card *c, CardDate cd);

void             card_set_time_expiry (Card *c, gint hour);
gint             card_get_time_expiry (Card *c);

void             card_set_expired (Card *c, gboolean expired);
gboolean         card_get_expired (Card *c);

void             card_set_n_tests (Card *c, guint n);
void             card_inc_n_tests (Card *c);
guint            card_get_n_tests (Card *c);

void             card_set_n_known (Card *c, guint n);
void             card_inc_n_known (Card *c);
guint            card_get_n_known (Card *c);

gfloat           card_get_score (Card *c);

void             card_reset_statistics (Card *c);

void             card_decrease_test_frequency (Card *c);

void             card_set_flagged (Card *c, gboolean flagged);
gboolean         card_get_flagged (Card *c);

void             card_to_top (Card *c);

Category*        category_new (const gchar *title);
void             category_free (Category *cat, gboolean free_cards);

void             category_set_title (Category *cat, const gchar *title);
const gchar*     category_get_title (Category *cat);
void             category_set_comment (Category *cat, const gchar *comment);
const gchar*     category_get_comment (Category *cat);

void             category_prepend_card (Category *cat, Card *c);
void             category_append_card (Category *cat, Card *c);
void             category_append_card_without_parent (Category *cat, Card *c);
void             category_remove_card (Category *cat, Card *c);
GList*           category_get_cards (Category *cat);
guint            category_get_n_cards (Category *cat);

guint            category_get_n_known (Category *cat);
guint            category_get_n_expired (Category *cat);
guint            category_get_n_untested (Category *cat);

void             category_set_card_style (Category *cat, gint style);
gint             category_get_card_style (Category *cat);

void             category_set_fixed_order (Category *cat, gboolean fixed);
gboolean         category_is_fixed_order (Category *cat);

gboolean         card_tested_today (Card *c);

Card*            card_get_details (Card *c);


gint     tag_cmp (Tag *a, Tag *b);

Tag*     tag_new (const gchar *name);
void     tag_free (Tag *t);
void     tag_ref (Tag *t);
void     tag_unref (Tag *t);
gint     tag_get_ref_count (Tag *t);
const gchar* tag_get_name (Tag *t);
void     tag_set_name (Tag *t, const gchar *name);

void     tag_list_free (GList *tlist);
GList*   tag_list_lookup_tag (GList *tlist, Tag *t);
GList*   tag_list_lookup_tag_by_name (GList *tlist, const gchar *name);
GList*   tag_list_add_tag (GList *tlist, Tag *t, gboolean *changed);
GList*   tag_list_add_new_tags (GList *tlist, const gchar *s, gboolean *changed);
GList*   tag_list_add_new_tags_with_card (Card *c, GList *tlist,
            const gchar *s, gboolean *changed);
GList*   tag_list_add_new_tag_with_card (Card *c, GList *tlist,
            const gchar *name, gboolean *changed);
GList*   tag_list_remove_tag (GList *tlist, Tag *t, gboolean *changed);
gboolean tag_list_match (GList *tlist1, GList *tlist2);
gchar*   tag_list_to_string (GList *tlist);


#endif  /* HAVE_CARD_H */

