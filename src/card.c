/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2008, 2009, 2015, 2016 Timothy Richard Musson
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
#include <glib/gi18n.h>

#include "main.h"
#include "card.h"
#include "prefs.h"
#include "textview.h"  /* For remove_card_markup. */


#define DEBUG_TAGS 0


static const gchar s_blank[] = "";


struct _Tag {
    gchar *name;
    gint  ref_count;
};


struct _Card {
    gchar       *front;         /* Front text. */
    gchar       *back;          /* Back text.  */
    gchar       *front_clean;   /* Front text without markup. */
    gchar       *back_clean;    /* Back text without markup.  */
    GList       *tags;          /* Pointers into File->tags. */
    CardStyle   style;          /* Appearance. */
    Group       group;          /* Difficulty. */
    GDate       *date[N_CARD_DATES];  /* Dates: created, tested, expiry */
    gint        time_expiry;    /* Hour of expiry (0..23).              */
    gboolean    expired;        /* Is this card due for testing now?    */
    guint       n_tests;        /* Number of times tested.              */
    guint       n_known;        /* Number of times scored as known.     */
    gboolean    flagged;        /* Is this card currently flagged?      */
    Category    *category;      /* This card's parent category.         */
};


struct _Category {
    gchar       *title;
    gchar       *comment;
    GList       *cards;      /* List of all cards in this category. */
    CardStyle   card_style;  /* Default card appearance. */
    gboolean    fixed_order; /* Should these cards be kept in order? */
};


Card*
card_new (void)
{
    Card *c;

    c = g_new0 (Card, 1);

    c->date[DATE_CREATED] = g_date_new ();
    date_today (c->date[DATE_CREATED]);

    return c;
}


Card*
card_copy (Card *src)
{
    Card *c;
    GList *cur;

    c = card_new ();

    c->front = g_strdup (src->front);
    c->back = g_strdup (src->back);
    c->front_clean = g_strdup (src->front_clean);
    c->back_clean = g_strdup (src->back_clean);

    for (cur = src->tags; cur != NULL; cur = cur->next)
        card_add_tag (c, TAG(cur));

    return c;
}


void
card_free (Card *c)
{
    gint i;

    g_free (c->front);
    g_free (c->back);
    g_free (c->front_clean);
    g_free (c->back_clean);

    for (i = 0; i < N_CARD_DATES; i++)
        if (c->date[i] != NULL)
            g_date_free (c->date[i]);

    tag_list_free (c->tags);

    g_free (c);
}


void
card_list_free (GList *list, gboolean free_cards)
{
    if (list != NULL) {

        if (free_cards)
            g_list_foreach (list, (GFunc)card_free, NULL);

        g_list_free (list);
    }
}


void
card_set_style (Card *c, gint style)
{
    c->style = style;
}


gint
card_get_style (Card *c)
{
    return c->style;
}


void
card_set_front (Card *c, const char *s)
{
    g_free (c->front);
    c->front = g_strdup (s);
    g_free (c->front_clean);
    c->front_clean = remove_card_markup (card_get_front (c));
}


void
card_set_back (Card *c, const char *s)
{
    g_free (c->back);
    c->back = g_strdup (s);
    g_free (c->back_clean);
    c->back_clean = remove_card_markup (card_get_back (c));
}


const gchar*
card_get_front (Card *c)
{
    return c->front ? c->front : s_blank;
}


const gchar*
card_get_front_without_markup (Card *c)
{
    return c->front_clean ? c->front_clean : s_blank;
}


const gchar*
card_get_back (Card *c)
{
    return c->back ? c->back : s_blank;
}


const gchar*
card_get_back_without_markup (Card *c)
{
    return c->back_clean ? c->back_clean : s_blank;
}


GList*
card_get_tags (Card *c)
{
    return c->tags;
}


gboolean
card_add_tag (Card *c, Tag *t)
{
    /* Return TRUE if this changes the file. */

    if (tag_list_lookup_tag (c->tags, t) != NULL)
        return FALSE;

    c->tags = g_list_insert_sorted (c->tags, t, (GCompareFunc)tag_cmp);
    tag_ref (t);

    return TRUE;
}


gboolean
card_set_tags (Card *c, GList *tlist)
{
    GList *p;

    /* Return TRUE if this changes the file. */

    if (tag_list_match (c->tags, tlist)) {
        return FALSE;
    }

    tag_list_free (c->tags);
    c->tags = NULL;

    for (p = tlist; p != NULL; p = p->next)
        card_add_tag (c, TAG(p));

    return TRUE;
}


gchar*
card_get_tags_as_string (Card *c)
{
    return tag_list_to_string (c->tags);
}


gboolean
card_remove_tag (Card *c, Tag *t)
{
    gboolean changed = FALSE;

    c->tags = tag_list_remove_tag (c->tags, t, &changed);

    return changed;
}


gboolean
card_is_blank (Card *c)
{
    return (card_front_is_blank (c) && card_back_is_blank (c));
}


void
card_switch_sides (Card *c)
{
    gchar *tmp;

    tmp = c->front;
    c->front = c->back;
    c->back = tmp;

    tmp = c->front_clean;
    c->front_clean = c->back_clean;
    c->back_clean = tmp;
}


void
card_set_group (Card *c, Group g)
{
    if (g < GROUP_NEW || g >= N_GROUPS)
        g = GROUP_NEW;

    c->group = g;
}


void
card_decrease_test_frequency (Card *c)
{
    /* Mark this card for less frequent testing. */

    if (c->group < GROUP_MAX)
        c->group++;
}


Group
card_get_group (Card *c)
{
    return c->group;
}


void
card_set_category (Card *c, Category *cat)
{
    c->category = cat;
}


Category*
card_get_category (Card *c)
{
    return c->category;
}


void
card_set_date (Card *c, CardDate cd, guint32 date)
{
    if (c->date[cd] == NULL)
        c->date[cd] = g_date_new ();

    g_date_set_julian (c->date[cd], date);
}


GDate*
card_get_gdate (Card *c, CardDate cd)
{
    return c->date[cd];
}


guint32
card_get_date (Card *c, CardDate cd)
{
    if (c->date[cd] == NULL)
        return G_DATE_BAD_JULIAN;

    return g_date_get_julian (c->date[cd]);
}


void
card_set_time_expiry (Card *c, gint hour)
{
    c->time_expiry = hour;
}


gint
card_get_time_expiry (Card *c)
{
    return c->time_expiry;
}


void
card_set_expired (Card *c, gboolean expired)
{
    c->expired = expired;
}


gboolean
card_get_expired (Card *c)
{
    return c->expired;
}


void
card_set_n_tests (Card *c, guint n)
{
    c->n_tests = n;
}


void
card_inc_n_tests (Card *c)
{
    c->n_tests++;
}


guint
card_get_n_tests (Card *c)
{
    return c->n_tests;
}


void
card_set_n_known (Card *c, guint n)
{
    c->n_known = n;
}


void
card_inc_n_known (Card *c)
{
    c->n_known++;
}


guint
card_get_n_known (Card *c)
{
    return c->n_known;
}


gfloat
card_get_score (Card *c)
{
    return c->n_tests == 0 ? 0 :
        (gfloat)card_get_n_known (c) / (gfloat)card_get_n_tests (c) * 100;
}


void
card_reset_statistics (Card *c)
{
    /* Forget this card's dates and scores. */

    if (c->date[DATE_TESTED] != NULL) {
        g_date_free (c->date[DATE_TESTED]);
        c->date[DATE_TESTED] = NULL;
    }

    if (c->date[DATE_EXPIRY] != NULL) {
        g_date_free (c->date[DATE_EXPIRY]);
        c->date[DATE_EXPIRY] = NULL;
    }

    c->time_expiry = 0;
    c->n_tests = 0;
    c->n_known = 0;
    c->expired = FALSE;
    c->group = GROUP_NEW;
}


#if 0

/* This version of card_mark_tested prevents cards from moving to a
 * higher group more than once in a single 24 hour period. */

void
card_mark_tested (Prefs *p, Card *c, GDate *date_tested, gint time_tested,
    gboolean known)
{
    /* Score this card as known or unknown, and update other statistics
     * accordingly. */

    guint32 jdate_tested;
    gboolean already_tested;


    /* Has the card been tested in the past 24 hours? */
    already_tested = card_tested_today (c);

    /* Record the number of tests. */
    card_inc_n_tests (c);

    /* Record date tested, and reset expiry date. */
    if (!known || !already_tested) {
        jdate_tested = g_date_get_julian (date_tested);
        card_set_date_tested (c, jdate_tested);
        card_set_date_expiry (c, jdate_tested);
        card_set_time_expiry (c, time_tested);
        card_set_expired (c, FALSE);
    }

    if (known) {

        GDate *expiry;

        /* Record the number of successful tests. */

        card_inc_n_known (c);

        if (!already_tested) {

            /* Set a new expiry date and mark the card for
             * less frequent testing. */

            expiry = card_get_gdate_expiry (c);
            g_date_add_days (expiry, prefs_get_schedule (p, card_get_group (c)));

            card_decrease_test_frequency (c);

        }

        return;
    }


    /* Card was unknown - mark it for frequent testing. */

    card_set_group (c, GROUP_1);
}

#else

void
card_mark_tested (Prefs *p, Card *c, GDate *date_tested, gint time_tested,
    gboolean known)
{
    /* Score this card as known or unknown, and update other statistics
     * accordingly. */

    guint32 jdate_tested;


    /* Record the number of tests. */
    card_inc_n_tests (c);

    /* Record date tested, and reset expiry date. */
    jdate_tested = g_date_get_julian (date_tested);
    card_set_date_tested (c, jdate_tested);
    card_set_date_expiry (c, jdate_tested);
    card_set_time_expiry (c, time_tested);
    card_set_expired (c, FALSE);

    if (known) {

        GDate *expiry;

        /* Record the number of successful tests. */

        card_inc_n_known (c);

        /* Set a new expiry date and mark the card for
         * less frequent testing. */

        expiry = card_get_gdate_expiry (c);
        g_date_add_days (expiry, prefs_get_schedule (p, card_get_group (c)));

        card_decrease_test_frequency (c);

        return;
    }


    /* Card was unknown - mark it for frequent testing. */

    card_set_group (c, GROUP_1);
}

#endif


void
card_set_flagged (Card *c, gboolean flagged)
{
    c->flagged = flagged;
}


gboolean
card_get_flagged (Card *c)
{
    return c->flagged;
}


void
card_to_top (Card *c)
{
    Category *cat;

    /* Move the card to the start of its category. */

    cat = c->category;
    category_remove_card (cat, c);
    category_prepend_card (cat, c);
}


Category*
category_new (const gchar *title)
{
    Category *cat;

    cat = g_new0 (Category, 1);
    cat->title = title ? g_strdup (title) : g_strdup (_("New Category"));
    return cat;
}


void
category_free (Category *cat, gboolean free_cards)
{
    g_free (cat->title);
    g_free (cat->comment);
    card_list_free (cat->cards, free_cards);
    g_free (cat);
}


void
category_set_title (Category *cat, const gchar *title)
{
    g_free (cat->title);
    cat->title = g_strdup (title);
}


const gchar*
category_get_title (Category *cat)
{
    return cat->title ? cat->title : s_blank;
}


void
category_set_comment (Category *cat, const gchar *comment)
{
    g_free (cat->comment);
    cat->comment = g_strdup (comment);
}


const gchar*
category_get_comment (Category *cat)
{
    return cat->comment ? cat->comment : s_blank;
}


void
category_append_card (Category *cat, Card *c)
{
    cat->cards = g_list_append (cat->cards, c);
    c->category = cat;
}


void
category_prepend_card (Category *cat, Card *c)
{
    cat->cards = g_list_prepend (cat->cards, c);
    c->category = cat;
}


void
category_append_card_without_parent (Category *cat, Card *c)
{
    /* For adding cards to a search result category. */
    cat->cards = g_list_append (cat->cards, c);
}


void
category_remove_card (Category *cat, Card *c)
{
    cat->cards = g_list_remove (cat->cards, c);
    c->category = NULL;
}


GList*
category_get_cards (Category *cat)
{
    if (cat == NULL)
        return NULL;

    return cat->cards;
}


guint
category_get_n_cards (Category *cat)
{
    return cat->cards ? g_list_length (cat->cards) : 0;
}


guint
category_get_n_known (Category *cat)
{
    GList *c;
    guint n = 0;

    for (c = category_get_cards (cat); c != NULL; c = c->next)
        if (CARD(c)->n_tests > 0 && !CARD(c)->expired)
            n++;

    return n;
}


guint
category_get_n_expired (Category *cat)
{
    GList *c;
    guint n = 0;

    for (c = category_get_cards (cat); c != NULL; c = c->next)
        if (CARD(c)->expired)
            n++;

    return n;
}


guint
category_get_n_untested (Category *cat)
{
    GList *c;
    guint n = 0;

    for (c = category_get_cards (cat); c != NULL; c = c->next)
        if (CARD(c)->n_tests == 0)
            n++;

    return n;
}


void
category_set_card_style (Category *cat, gint style)
{
    cat->card_style = style;
}


gint
category_get_card_style (Category *cat)
{
    return cat->card_style;
}


void
category_set_fixed_order (Category *cat, gboolean fixed)
{
    cat->fixed_order = fixed;
}


gboolean
category_is_fixed_order (Category *cat)
{
    return cat->fixed_order;
}


gboolean
card_tested_today (Card *c)
{
    GDate date;

    /* Return TRUE if this card was tested in the past 24 hours. */

    if (card_get_date_tested (c) == G_DATE_BAD_JULIAN)
        return FALSE;

    date_today (&date);

    if (g_date_compare (card_get_gdate_tested (c), &date) == 0)
        return TRUE;

    g_date_subtract_days (&date, 1);

    if (g_date_compare (card_get_gdate_tested (c), &date) == 0
        && get_current_hour () < card_get_time_expiry (c))
        return TRUE;

    return FALSE;
}


Card*
card_get_details (Card *c)
{
    GString  *gstr;
    gchar    crt[11], tst[11], exp[11];
    Category *cat;
    Card     *details;

    /* Returns a newly allocated card for the display of details in the
     * card editor and quiz windows. The new card should be freed when
     * no longer needed. */

    cat = card_get_category (c);

    details = card_new ();
    card_set_style (details, CARD_STYLE_SENTENCES);

    date_str (crt, card_get_gdate_created (c));
    date_str (tst, card_get_gdate_tested (c));
    date_str (exp, card_get_gdate_expiry (c));

    gstr = g_string_new ("<center>");
    g_string_append (gstr, _("<big>Card Details</big>\n"));
    g_string_append_c (gstr, '\n');
    g_string_append_printf (gstr, _("<b>Category:</b> %s\n"),
        category_get_title (cat));
    g_string_append_printf (gstr, _("<b>Success rate:</b> %.0f%% (%d/%d)\n"),
        card_get_score (c), card_get_n_known (c), card_get_n_tests (c));
    g_string_append_printf (gstr, _("<b>Current box:</b> %d\n"),
        card_get_group (c));
    g_string_append_printf (gstr, _("<b>Date created:</b> %s\n"), crt);
    g_string_append_printf (gstr, _("<b>Date last tested:</b> %s\n"), tst);
    g_string_append_printf (gstr, _("<b>Date of expiry:</b> %s"), exp);
    g_string_append_printf (gstr, " (%02d:00)\n", card_get_time_expiry (c));
    g_string_append (gstr, "</center>");

    card_set_front (details, gstr->str);

    g_string_free (gstr, TRUE);
    
    return details;
}


gint
tag_cmp (Tag *a, Tag *b)
{
    return strcmp (tag_get_name (a), tag_get_name (b));
}


Tag*
tag_new (const gchar *name)
{
    Tag *t;

    t = g_new (Tag, 1);
    t->name = g_strdup (name);
    t->ref_count = 0;

#if DEBUG_TAGS
    g_printerr ("tag_new: %s\n", name);
#endif

    return t;
}


void
tag_free (Tag *t)
{
#if DEBUG_TAGS
    g_printerr ("tag_free: %s\n", t->name);
#endif
    g_free (t->name);
    g_free (t);
}


void
tag_ref (Tag *t)
{
    g_assert (t != NULL);
    t->ref_count++;

#if 0
    g_printerr ("tag_ref (%d) %s\n", t->ref_count, t->name);
#endif
}


void
tag_unref (Tag *t)
{
    g_assert (t != NULL);
    g_assert (t->ref_count > 0);

    t->ref_count--;

#if DEBUG_TAGS
    g_printerr ("tag_unref: %s (%d)\n", t->name, t->ref_count);
#endif

#if 1
    /* XXX: This should be removed once everything's well tested. */
    if (t->ref_count < 0)
        g_warning ("tag_unref: ref_count %d: %s\n", t->ref_count, t->name);
#endif

    if (t->ref_count == 0)
        tag_free (t);
}


void
tag_set_name (Tag *t, const gchar *name)
{
    g_free (t->name);
    t->name = g_strdup (name);
}


const gchar*
tag_get_name (Tag *t)
{
    return t->name;
}


gint
tag_get_ref_count (Tag *t)
{
    return t->ref_count;
}


void
tag_list_free (GList *tlist)
{
    if (tlist == NULL)
        return;

    g_list_foreach (tlist, (GFunc)tag_unref, NULL);
    g_list_free (tlist);
}


GList*
tag_list_lookup_tag (GList *tlist, Tag *t)
{
    GList *p;

    for (p = tlist; p != NULL; p = p->next)
        if (p->data == t)
            return p;

    return NULL;
}


GList*
tag_list_lookup_tag_by_name (GList *tlist, const gchar *name)
{
    GList *p;

    for (p = tlist; p != NULL; p = p->next)
        if (strcmp (TAG(p)->name, name) == 0)
            return p;

    return NULL;
}


GList*
tag_list_add_tag (GList *tlist, Tag *t, gboolean *changed)
{
    if (tag_list_lookup_tag (tlist, t) == NULL) {
        *changed |= TRUE;
        tag_ref (t);
        tlist = g_list_insert_sorted (tlist, t, (GCompareFunc)tag_cmp);
    }

    return tlist;
}


static GList*
tag_list_add_new_tag (GList *tlist, const gchar *name, gboolean *changed)
{
    if (tag_list_lookup_tag_by_name (tlist, name) == NULL) {
        Tag *t = tag_new (name);
        *changed |= TRUE;
        tag_ref (t);
        tlist = g_list_insert_sorted (tlist, t, (GCompareFunc)tag_cmp);
    }

    return tlist;
}


GList*
tag_list_add_new_tags (GList *tlist, const gchar *s, gboolean *changed)
{
    gchar **strv, **name;

    strv = name = g_strsplit (s, " ", 0);

    while (*name != NULL) {

        if (*name[0] != '\0')
            tlist = tag_list_add_new_tag (tlist, *name, changed);

        name++;
    }
    g_strfreev (strv);

    return tlist;
}


GList*
tag_list_add_new_tag_with_card (Card *c, GList *tlist, const gchar *name,
    gboolean *changed)
{
    GList *item;
    Tag *t;

    item = tag_list_lookup_tag_by_name (tlist, name);
    if (item != NULL) {
        t = TAG(item);
    }
    else {
        t = tag_new (name);
        tlist = g_list_insert_sorted (tlist, t, (GCompareFunc)tag_cmp);
        tag_ref (t);
        *changed |= TRUE;
    }

    *changed |= card_add_tag (c, t);

    return tlist;
}


GList*
tag_list_add_new_tags_with_card (Card *c, GList *tlist, const gchar *s,
        gboolean *changed)
{
    gchar **strv, **name;

    strv = name = g_strsplit (s, " ", 0);

    while (*name != NULL) {

        if (*name[0] != '\0')
            tlist = tag_list_add_new_tag_with_card (c, tlist, *name, changed);

        name++;
    }
    g_strfreev (strv);

    return tlist;
}


GList*
tag_list_remove_tag (GList *tlist, Tag *t, gboolean *changed)
{
    GList *item;

    item = tag_list_lookup_tag (tlist, t);
    if (item != NULL) {
        tag_unref (t);
        tlist = g_list_delete_link (tlist, item);
        *changed |= TRUE;
    }

    return tlist;
}


gboolean
tag_list_match (GList *tlist1, GList *tlist2)
{
    /* Return FALSE if the lists differ, otherwise TRUE. */

    if (tlist1 == NULL && tlist2 == NULL)
        return TRUE;

    if (tlist1 == NULL && tlist2 != NULL)
        return FALSE;

    if (tlist1 != NULL && tlist2 == NULL)
        return FALSE;

    if (g_list_length (tlist1) != g_list_length (tlist2))
        return FALSE;

    /* This assumes both lists are sorted. */
    while (tlist1 != NULL) {

        if (tlist1->data != tlist2->data)
            return FALSE;

        tlist1 = tlist1->next;
        tlist2 = tlist2->next;
    }

    return TRUE;
}


gchar*
tag_list_to_string (GList *tlist)
{
    GList *p;
    GString *gs;
    gchar *ret;

    gs = g_string_new ("");

    for (p = tlist; p != NULL; p = p->next) {
        g_string_append (gs, TAG(p)->name);
        if (p->next != NULL)
            g_string_append_c (gs, ' ');
    }

    ret = gs->str;
    g_string_free (gs, FALSE);

    return ret;
}

