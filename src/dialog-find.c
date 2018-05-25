/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2008, 2009, 2016 Timothy Richard Musson
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
#include <glade/glade.h>

#include "main.h"
#include "file.h"
#include "app-window.h"
#include "dialog-editor.h"
#include "dialog-category-properties.h"
#include "dialog-find.h"


#define HISTORY_LENGTH   10


typedef const gchar* (*GetCardTextFunc)(Card *card);
typedef gchar* (*StrStrFunc)(const char *haystack, const char *needle);
typedef gint (*StrCmpFunc)(const char *s1, const char *s2);


typedef struct {

    Ignuit              *ig;
    GtkWidget           *window;
    GtkWidget           *combo_entry;
    GSList              *face_group;
    GtkWidget           *t_regex;
    GtkWidget           *t_case;
    GtkWidget           *t_markup;

    StrStrFunc          my_strstr;
    StrCmpFunc          my_strcmp;

} Dialog;


static Dialog *dialog = NULL;


enum {
    RADIO_FRONTS = 0,
    RADIO_BACKS,
    RADIO_BOTH,
    RADIO_TAGS,
    N_RADIO_BUTTONS
};


static gchar*
utf8_strcasestr (const gchar *haystack, const gchar *needle)
{
    gchar *haystack_fold, *needle_fold, *ret;

    /* Return non-NULL if haystack contains the substring needle, case
     * insensitive. Note that the returned value *must not* be used for
     * anything other than checking whether or not it's NULL. */

    haystack_fold = g_utf8_casefold (haystack, -1);
    needle_fold = g_utf8_casefold (needle, -1);

    ret = strstr (haystack_fold, needle_fold);

    g_free (haystack_fold);
    g_free (needle_fold);

    return ret;
}


static gint
utf8_strcasecmp (const gchar *s1, const gchar *s2)
{
    gchar *s1_fold, *s2_fold;
    gint ret;

    s1_fold = g_utf8_casefold (s1, -1);
    s2_fold = g_utf8_casefold (s2, -1);

    ret = g_utf8_collate (s1_fold, s2_fold);

    g_free (s1_fold);
    g_free (s2_fold);

    return ret;
}


static gint
get_radio_selection (GSList *group, gint n)
{
    GSList *cur;

    for (cur = group; cur != NULL; cur = cur->next) {

        n--;

        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(cur->data)))
            return n;

    }
    g_assert_not_reached ();
}


void
cb_r_tags_toggled (GtkToggleButton *widget, Dialog *d)
{
    gboolean active;

    active = gtk_toggle_button_get_active (widget);
    gtk_widget_set_sensitive (d->t_markup, !active);
}


static void
search_tags (Dialog *d, GRegex *re, const gchar *text)
{
    GList *p, *q, *tlist_card, *tlist = NULL;
    gboolean changed = FALSE;


    if (text[0] == '\0') {

        /* Find cards lacking tags. */

        for (p = file_get_cards (d->ig->file); p != NULL; p = p->next) {
            if (card_get_tags (CARD(p)) == NULL) {
                file_add_search_card (d->ig->file, CARD(p));
            }
        }
        return;
    }


    /* Get a list of tags that match the query. */

    for (p = file_get_tags (d->ig->file); p != NULL; p = p->next) {
        if (re != NULL) {
            if (g_regex_match (re, tag_get_name (TAG(p)), 0, NULL)) {
                tlist = tag_list_add_tag (tlist, TAG(p), &changed);
            }
        }
        else {
            if (d->my_strcmp (tag_get_name (TAG(p)), text) == 0) {
                tlist = tag_list_add_tag (tlist, TAG(p), &changed);
                /* This'll be the only one, so no need to continue. */
                break;
            }
        }
    }


    /* Find all cards marked with those tags. */

    for (p = file_get_cards (d->ig->file); p != NULL; p = p->next) {

        tlist_card = card_get_tags (CARD(p));

        for (q = tlist; q != NULL; q = q->next) {
            if (tag_list_lookup_tag (tlist_card, TAG(q)) != NULL) {
                file_add_search_card (d->ig->file, CARD(p));
                /* Done with this card, so: */
                break;
            }
        }

    }

    tag_list_free (tlist);
}


static void
search_cards (Dialog *d, gboolean fronts, gboolean backs, GRegex *re,
    const gchar *text)
{
    GetCardTextFunc get_front;
    GetCardTextFunc get_back;
    GList *p;
    gboolean hit;
    Category *cat;
    Card *c;


    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(d->t_markup))) {
        get_front = card_get_front;
        get_back = card_get_back;
    }
    else {
        get_front = card_get_front_without_markup;
        get_back = card_get_back_without_markup;
    }

    for (p = file_get_cards (d->ig->file); p != NULL; p = p->next) {

        hit = FALSE;

        c = CARD(p);

        cat = card_get_category (c);

        if (fronts) {

            if (text[0] == '\0') {
                /* Find cards with no front text. */
                hit = card_front_is_blank (c);
            }
            else if (re != NULL) {
                hit = g_regex_match (re, get_front (c), 0, NULL);
            }
            else {
                hit = (d->my_strstr (get_front (c), text) != NULL);
            }
        }

        if (backs) {

            if (text[0] == '\0') {
                /* Find cards with no back text. */
                hit |= card_back_is_blank (c);
            }
            else if (re != NULL) {
                hit |= g_regex_match (re, get_back (c), 0, NULL);
            }
            else {
                hit |= (d->my_strstr (get_back (c), text) != NULL);
            }
        }

        if (hit)
            file_add_search_card (d->ig->file, c);

    }
}


static void
cb_find (GtkWidget *widget, Dialog *d)
{
    GRegex *re = NULL;
    GtkComboBox *combo;
    gboolean match_case;
    gchar *text;
    GList *cards;
    Category *cat;
    gint i;


    combo = GTK_COMBO_BOX(d->combo_entry);

    if ((text = gtk_combo_box_get_active_text (combo)) == NULL)
        return;

    dialog_editor_check_changed ();
    dialog_category_properties_close ();

    if (text[0] != '\0') {

        GList *history = d->ig->recent_search_terms;
        gboolean is_duplicate = FALSE;

        /* Add this query to search history, if it's not already there. */

        while (history != NULL) {

            gchar *past_text = history->data;

            if (!strcmp (past_text, text)) {
                is_duplicate = TRUE;
                break;
            }

            history = history->next;
        }

        if (!is_duplicate) {

            gtk_combo_box_prepend_text (combo, text);
            gtk_combo_box_remove_text (combo, HISTORY_LENGTH);

            d->ig->recent_search_terms = g_list_prepend
                (d->ig->recent_search_terms, text);

            if (g_list_length (d->ig->recent_search_terms) > HISTORY_LENGTH) {

                GList *item;

                item = g_list_last (d->ig->recent_search_terms);
                d->ig->recent_search_terms = g_list_remove_link
                    (d->ig->recent_search_terms, item);

                g_free (item->data);
            }
        }
    }

    /* Clear any previous search results. */
    file_clear_search (d->ig->file, TRUE);


    match_case = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(d->t_case));

    d->my_strstr = match_case ? strstr : utf8_strcasestr;
    d->my_strcmp = match_case ? strcmp : utf8_strcasecmp;

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(d->t_regex)) == TRUE) {

        GError *err = NULL;

        re = g_regex_new (text, match_case ? 0 : G_REGEX_CASELESS, 0, &err);
        if (err != NULL) {
            g_warning ("%s\n", err->message);
            g_error_free (err);
            re = NULL; /* Fall back to plain search. */
        }
    }


    i = get_radio_selection (d->face_group, N_RADIO_BUTTONS);

    if (i == RADIO_TAGS) {
        search_tags (d, re, text);
    }
    else {

        gboolean fronts;
        gboolean backs;

        fronts = (i == RADIO_FRONTS || i == RADIO_BOTH);
        backs = (i == RADIO_BACKS || i == RADIO_BOTH);

        search_cards (d, fronts, backs, re, text);
    }


    if (re != NULL)
        g_regex_unref (re);

    /* If text wasn't added to ig->recent_search_terms, free it now. */
    if (text[0] == '\0')
        g_free (text);

    cat = file_get_search (d->ig->file);
    cards = category_get_cards (cat);

    app_window_refresh_card_pane (d->ig, cards);
    app_window_select_category (d->ig, NULL);
    app_window_select_card (d->ig, cards);

    file_set_current_category (d->ig->file, cat);
    file_set_current_item (d->ig->file, cards);


    /* Update main window controls and appbar. */

    gtk_widget_set_sensitive (d->ig->m_remove_category, FALSE);
    gtk_widget_set_sensitive (d->ig->b_remove_category, FALSE);
    gtk_widget_set_sensitive (d->ig->m_category_properties, FALSE);
    gtk_widget_set_sensitive (d->ig->m_add_card, FALSE);
    gtk_widget_set_sensitive (d->ig->b_add_card, FALSE);
    gtk_widget_set_sensitive (d->ig->m_edit_tags, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_flag, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_switch_sides, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_reset_stats, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_paste_card, FALSE);

    gtk_widget_set_sensitive (d->ig->m_category_popup_rename, FALSE);
    gtk_widget_set_sensitive (d->ig->m_category_popup_remove, FALSE);
    gtk_widget_set_sensitive (d->ig->m_category_popup_toggle_fixed_order, FALSE);
    gtk_widget_set_sensitive (d->ig->m_category_popup_properties, FALSE);

    gtk_widget_set_sensitive (d->ig->m_card_popup_edit_tags, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_card_popup_flag, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_card_popup_switch_sides, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_card_popup_reset_stats, cards != NULL);
    gtk_widget_set_sensitive (d->ig->m_card_popup_paste, FALSE);

    app_window_update_appbar (d->ig);

    dialog_editor_tweak (ED_TWEAK_ALL);
}


static void
cb_destroy (GtkWidget *widget, Dialog *d)
{
    prefs_set_find_with_regex (d->ig->prefs,
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(d->t_regex)));

    g_free (d);
    dialog = NULL;
}


static void
cb_close (GtkWidget *widget, Dialog *d)
{
    gtk_widget_destroy (d->window);
}


void
dialog_find (Ignuit *ig)
{
    Dialog    *d;
    GtkWidget *w, *b_close, *b_find, *hbox, *label, *entry, *r_tags;
    gchar     *glade_file;
    GladeXML  *glade_xml;
    GList     *cur;


    if (dialog != NULL) {
        gtk_window_present (GTK_WINDOW(dialog->window));
        return;
    }

    glade_file = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_FIND, TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_FIND);
        return;
    }

    dialog = d = g_new0 (Dialog, 1);
    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;

    d->window = glade_xml_get_widget (glade_xml, "dialog");

    label = glade_xml_get_widget (glade_xml, "label_search");

    b_close = glade_xml_get_widget (glade_xml, "b_close");
    b_find = glade_xml_get_widget (glade_xml, "b_find");

    hbox = glade_xml_get_widget (glade_xml, "hbox1");
    d->combo_entry = gtk_combo_box_entry_new_text ();
    gtk_box_pack_start (GTK_BOX(hbox), d->combo_entry, TRUE, TRUE, 0);
    gtk_widget_show (d->combo_entry);

    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->combo_entry);

    for (cur = ig->recent_search_terms; cur != NULL; cur = cur->next)
        gtk_combo_box_append_text (GTK_COMBO_BOX(d->combo_entry),
            (gchar*)cur->data);

    entry = gtk_bin_get_child (GTK_BIN(d->combo_entry));

    w = glade_xml_get_widget (glade_xml, "r_fronts");
    d->face_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON(w));

    w = glade_xml_get_widget (glade_xml, "r_both");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(w), TRUE);

    r_tags = glade_xml_get_widget (glade_xml, "r_tags");

    d->t_regex = glade_xml_get_widget (glade_xml, "toggle_regex");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(d->t_regex),
        prefs_get_find_with_regex (d->ig->prefs));

    d->t_case = glade_xml_get_widget (glade_xml, "toggle_case");
    d->t_markup = glade_xml_get_widget (glade_xml, "toggle_markup");

    g_signal_connect (G_OBJECT(d->window), "destroy",
        G_CALLBACK(cb_destroy), d);

    g_signal_connect (G_OBJECT(entry), "activate",
        G_CALLBACK(cb_find), d);
    g_signal_connect (G_OBJECT(r_tags), "toggled",
        G_CALLBACK(cb_r_tags_toggled), d);
    g_signal_connect (G_OBJECT(b_close), "clicked",
        G_CALLBACK(cb_close), d);
    g_signal_connect (G_OBJECT(b_find), "clicked",
        G_CALLBACK(cb_find), d);

    gtk_window_set_transient_for (GTK_WINDOW(d->window),
        GTK_WINDOW(ig->app));

    gtk_widget_show (d->window);

    g_object_unref (G_OBJECT(glade_xml));
}

