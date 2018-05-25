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
#include <glib/gi18n.h>
#include <glade/glade.h>

#include "main.h"
#include "file.h"
#include "prefs.h"
#include "dialog-quiz.h"
#include "app-window.h"
#include "textview.h"
#include "audio.h"


#define CUR_IS_LAST(d)          ((d)->cur->data == (d)->last)
#define QUIZITEM(glist_item)    ((QuizItem*)(glist_item)->data)
#define QCARD(q)                (QUIZITEM(q)->card)


/* XXX: this belongs in card.h */
void card_mark_tested (Prefs *p, Card *c, GDate *date_tested,
    gint time_tested, gboolean known);



typedef struct _QuizItem QuizItem;

struct _QuizItem {

    Card *card;

    gboolean tested;
    gboolean known;
    gboolean reverse;

    gchar *note;

};


typedef struct {

    Ignuit              *ig;
    GtkWidget           *window;

    GtkNotebook         *notebook;
    GtkTextView         *textview[3];
    GtkTextBuffer       *textbuf[3];

    GList               *cardview_selected;

    GtkWidget           *hbox_answer_bar;
    GtkWidget           *entry_answer_bar;
    GtkWidget           *b_clear_answer_bar;

    GtkWidget           *m_prev;
    GtkWidget           *m_next;
    GtkCheckMenuItem    *m_known;
    GtkCheckMenuItem    *m_unknown;
    GtkCheckMenuItem    *m_flip;
    GtkCheckMenuItem    *m_info;
    GtkCheckMenuItem    *m_answer_bar;
    GtkWidget           *m_listen_front;
    GtkWidget           *m_listen_back;
    GtkCheckMenuItem    *m_auto_listen_front;
    GtkCheckMenuItem    *m_auto_listen_back;
    GtkCheckMenuItem    *m_flag;
    GtkToggleButton     *b_info;
    GtkWidget           *b_prev;
    GtkWidget           *b_next;
    GtkWidget           *chk_store;
    GtkToggleButton     *b_known;
    GtkToggleButton     *b_unknown;
    GtkToggleButton     *b_flip;

    GtkStatusbar        *statusbar;
    guint               sbcid;

    GList               *quizitems;
    GList               *cur;

    QuizItem            *last;       /* The final "End of quiz" message. */

    gint                n_quizitems;
    gint                n;

    QuizMode            quiz_mode;

    GDate               date_tested;
    gint                time_tested;

} Dialog;


static void cb_m_flag (GtkCheckMenuItem *widget, Dialog *d);
static void toggle_ui_known (Dialog *d, gboolean active);
static void toggle_ui_unknown (Dialog *d, gboolean active);
static void toggle_ui_info (Dialog *d, gboolean active);
static void toggle_ui_flip (Dialog *d, gboolean active);
static void update_title (void);


static Dialog *dialog = NULL;


static QuizItem*
quizitem_new (Card *card)
{
    QuizItem *q;

    q = g_new0 (QuizItem, 1);
    q->card = card;

    return q;
}


static void
quizitem_free (QuizItem *item)
{
    g_free (item->note);
    g_free (item);
}


static void
quizitem_set_note (Dialog *d)
{
    QuizItem *item;

    item = QUIZITEM(d->cur);

    g_free (item->note);
    item->note = g_strdup (gtk_entry_get_text (GTK_ENTRY(d->entry_answer_bar)));
}


static GList*
shuffle_quizitems (GRand *grand, GList *list)
{
    GList *new = NULL;
    gint len, x;
    gpointer data;


    if (g_list_length (list) < 2)
        return list;

    while ((len = g_list_length (list)) > 0) {
        x = g_rand_int_range (grand, 0, len);
        data = g_list_nth_data (list, x);
        new = g_list_append (new, data);
        list = g_list_remove (list, data);
    }
    g_list_free (list);

    return new;
}


static GList*
quizitems_from_cards (GList *list, QuizInfo *info)
{
    GList *cur;
    GList *quizitems = NULL;
    gboolean add_it;


    for (cur = list; cur != NULL; cur = cur->next) {

        switch (info->card_selection) {
        case QUIZ_ALL_CARDS:
            add_it = TRUE;
            break;
        case QUIZ_NEW_CARDS:
            add_it = (card_get_group (CARD(cur)) == GROUP_NEW);
            break;
        case QUIZ_EXPIRED_CARDS:
            add_it = card_get_expired (CARD(cur));
            break;
        case QUIZ_NEW_AND_EXPIRED_CARDS:
            add_it = (card_get_group (CARD(cur)) == GROUP_NEW ||
                card_get_expired (CARD(cur)));
            break;
        case QUIZ_FLAGGED_CARDS:
            add_it = card_get_flagged (CARD(cur));
            break;
        case QUIZ_SELECTED_CARDS:
            add_it = TRUE;
            break;
        default:
            g_assert_not_reached ();
            break;
        }

        if (add_it)
            quizitems = g_list_append (quizitems, quizitem_new (cur->data));

    }
    return quizitems;
}


static GList*
get_quizitems (Dialog *d, GRand *grand)
{
    Category *cat;
    GList *cards = NULL;
    GList *cur, *quizitems;


    /* Build a list of cards to be tested. */

    if (d->ig->quizinfo.card_selection == QUIZ_SELECTED_CARDS) {

        cards = d->cardview_selected;

    }
    else {

        switch (d->ig->quizinfo.category_selection) {
        case QUIZ_CURRENT_CATEGORY:
            cat = file_get_current_category (d->ig->file);
            if (category_get_n_cards (cat) > 0)
                cards = category_get_cards (cat);
            break;
        case QUIZ_ALL_CATEGORIES:
            cards = file_get_cards (d->ig->file);
            break;
        default:
            g_assert_not_reached ();
            break;
        }

    }

    if (!cards) { return NULL; }

    quizitems = quizitems_from_cards (cards, &d->ig->quizinfo);
    if (!d->ig->quizinfo.in_order) {
        quizitems = shuffle_quizitems (grand, quizitems);
    }

    switch (d->ig->quizinfo.face_selection) {
    case QUIZ_FACE_BACK:
        for (cur = quizitems; cur != NULL; cur = cur->next)
            QUIZITEM(cur)->reverse = TRUE;
        break;
    case QUIZ_FACE_RANDOM:
        for (cur = quizitems; cur != NULL; cur = cur->next)
            QUIZITEM(cur)->reverse = g_rand_int_range (grand, 0, 2);
        break;
    default:
        break;
    }

    return quizitems;
}


static void
store_quiz_results (Dialog *d, gboolean store_statistics)
{
    GList *cur;


    /* Update the statistics of each tested card. */

    for (cur = d->quizitems; cur != NULL; cur = cur->next) {

        if (QUIZITEM(cur)->tested) {

            if (store_statistics)
                card_mark_tested (d->ig->prefs, QCARD(cur),
                        &d->date_tested, d->time_tested, QUIZITEM(cur)->known);

            /* Move cards marked as "unknown" to the top in their categories. */
            if (!QUIZITEM(cur)->known) {
                Card *c = QCARD(cur);
                if (!category_is_fixed_order (card_get_category(c))) {
                    card_to_top (c);
                }
            }

            ig_file_changed (d->ig);

        }
    }

    file_check_expired (d->ig->file);

    app_window_refresh_card_pane (d->ig,
        file_get_current_category_cards (d->ig->file));

    app_window_update_appbar (d->ig);
}


static void
statusbar_set_message (Dialog *d, const gchar *text)
{
    gtk_statusbar_pop (d->statusbar, d->sbcid);
    gtk_statusbar_push (d->statusbar, d->sbcid, text);
}


static void
show_current_card (Dialog *d, gboolean quiet)
{
    Card *c;
    CardStyle style;
    GtkJustification just;
    GdkColor *color;
    gboolean active, reverse;
    const gchar *text;


    audio_free_all ();

    c = QCARD(d->cur);

    if (CUR_IS_LAST(d)) {

        /* It's the "end of quiz" message. */

        color = prefs_get_color (d->ig->prefs, COLOR_CARD_BG_END);

        statusbar_set_message (d, "");
        gtk_widget_grab_focus (d->b_prev);

    }
    else {

        gchar *s;

        color = prefs_get_color (d->ig->prefs, COLOR_CARD_BG);

        s = g_strdup_printf (_("Card %d of %d. Category: %s"),
            d->n,
            d->n_quizitems,
            category_get_title (card_get_category (c)));
        statusbar_set_message (d, s);
        g_free (s);

    }

    style = file_card_get_card_style (d->ig->file, c);
    just = card_style_get_justification (style);

    gtk_text_view_set_justification (GTK_TEXT_VIEW(d->textview[FRONT]), just);
    gtk_text_view_set_justification (GTK_TEXT_VIEW(d->textview[BACK]), just);

    reverse = !CUR_IS_LAST(d) && QUIZITEM(d->cur)->reverse;

    text = reverse ? card_get_back (c) : card_get_front (c);
    textview_display_with_markup (d->ig, d->textview[FRONT], text, style, FRONT);

    text = reverse ? card_get_front (c) : card_get_back (c);
    textview_display_with_markup (d->ig, d->textview[BACK], text, style, BACK);

    if (!CUR_IS_LAST(d)) {

        Card *c_info;

        /* Card details. */

        c_info = card_get_details (c);

        style = card_get_style (c_info);
        just = card_style_get_justification (style);
        gtk_text_view_set_justification (GTK_TEXT_VIEW(d->textview[INFO]),
            just);

        textview_display_with_markup (d->ig, d->textview[INFO],
            card_get_front (c_info), style, INFO);

        card_free (c_info);
    }


    /* Buttons and background colours. */

    active = QUIZITEM(d->cur)->tested && QUIZITEM(d->cur)->known;
    toggle_ui_known (d, active);
    if (active)
        color = prefs_get_color (d->ig->prefs, COLOR_CARD_BG_KNOWN);

    active = QUIZITEM(d->cur)->tested && !QUIZITEM(d->cur)->known;
    toggle_ui_unknown (d, active);
    if (active)
        color = prefs_get_color (d->ig->prefs, COLOR_CARD_BG_UNKNOWN);

    gtk_widget_modify_base (GTK_WIDGET(d->textview[FRONT]),
        GTK_STATE_NORMAL, color);
    gtk_widget_modify_base (GTK_WIDGET(d->textview[BACK]),
        GTK_STATE_NORMAL, color);

    g_signal_handlers_block_by_func (G_OBJECT(d->m_flag),
        G_CALLBACK(cb_m_flag), d);
    gtk_check_menu_item_set_active (d->m_flag, card_get_flagged (c));
    g_signal_handlers_unblock_by_func (G_OBJECT(d->m_flag),
        G_CALLBACK(cb_m_flag), d);

    update_title ();

    if (QUIZITEM(d->cur)->note != NULL)
        gtk_entry_set_text (GTK_ENTRY(d->entry_answer_bar),
            QUIZITEM(d->cur)->note);
    else
        gtk_entry_set_text (GTK_ENTRY(d->entry_answer_bar), "");

    if (!quiet && (prefs_get_auto_listen (d->ig->prefs) & LISTEN_FRONT))
        audio_play_side (FRONT);

}


static void
set_interface_sensitivity (Dialog *d)
{
    gboolean last;

    last = CUR_IS_LAST(d);

    gtk_widget_set_sensitive (d->b_prev, d->cur->prev != NULL);
    gtk_widget_set_sensitive (d->b_next, d->cur->next != NULL);

    gtk_widget_set_sensitive (d->m_prev, d->cur->prev != NULL);
    gtk_widget_set_sensitive (d->m_next, d->cur->next != NULL);

    gtk_widget_set_sensitive (GTK_WIDGET(d->b_unknown), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->b_known), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->b_flip), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->b_info), !last);

    gtk_widget_set_sensitive (GTK_WIDGET(d->m_unknown), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->m_known), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->m_flip), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->m_info), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->m_flag), !last);

    gtk_widget_set_sensitive (GTK_WIDGET(d->entry_answer_bar), !last);
    gtk_widget_set_sensitive (GTK_WIDGET(d->b_clear_answer_bar), !last);

    gtk_widget_set_sensitive (GTK_WIDGET(d->chk_store),
        d->quiz_mode == QUIZ_MODE_NORMAL && d->n_quizitems > 0);
}


static gboolean
step_forward (Dialog *d)
{
    /* Switch to the next card, or return FALSE. */

    if (d->cur->next) {

        quizitem_set_note (d);

        d->cur = d->cur->next;
        d->n = d->n + 1;

        toggle_ui_info (d, FALSE);
        toggle_ui_flip (d, FALSE);
        gtk_notebook_set_current_page (d->notebook, FRONT);

        set_interface_sensitivity (d);

        return TRUE;

    }
    return FALSE;
}


static gboolean
step_back (Dialog *d)
{
    /* Switch to the previous card, or return FALSE. */

    if (d->cur->prev) {

        quizitem_set_note (d);

        d->cur = d->cur->prev;
        d->n = d->n - 1;

        toggle_ui_info (d, FALSE);
        toggle_ui_flip (d, FALSE);
        gtk_notebook_set_current_page (d->notebook, FRONT);

        if (d->cur->prev == NULL) {
            /* Can't go any further back, so focus the "Next" button. */
            gtk_widget_grab_focus (d->b_next);
        }

        set_interface_sensitivity (d);

        return TRUE;

    }
    return FALSE;
}


static void
cb_m_info (GtkCheckMenuItem *widget, Dialog *d)
{
    gboolean active;
    gint page;


    active =  gtk_check_menu_item_get_active (widget);
    toggle_ui_info (d, active);

    if (active)
        page = INFO;
    else if (gtk_check_menu_item_get_active (d->m_flip))
        page = BACK;
    else
        page = FRONT;
   
    gtk_notebook_set_current_page (d->notebook, page);
}


static void
cb_m_answer_bar (GtkCheckMenuItem *widget, Dialog *d)
{
    gboolean active;

    active =  gtk_check_menu_item_get_active (widget);
    if (active) {
        gtk_widget_show (d->hbox_answer_bar);
        gtk_widget_grab_focus (d->entry_answer_bar);
    }
    else {
        gtk_widget_hide (d->hbox_answer_bar);
    }

    prefs_set_quiz_answer_bar_visible (d->ig->prefs, active);
}


static void
cb_b_close_answer_bar (GtkWidget *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_answer_bar, FALSE);
    gtk_widget_hide (d->hbox_answer_bar);

    prefs_set_quiz_answer_bar_visible (d->ig->prefs, FALSE);
}


static void
cb_b_clear_answer_bar (GtkWidget *widget, Dialog *d)
{
    gtk_entry_set_text (GTK_ENTRY(d->entry_answer_bar), "");
}


static void
cb_answer_bar_activate (GtkWidget *widget, Dialog *d)
{
    gboolean flipped;

    flipped = gtk_check_menu_item_get_active (d->m_flip);
    gtk_check_menu_item_set_active (d->m_flip, !flipped);
    gtk_widget_grab_focus (d->entry_answer_bar);
}


static void
cb_b_info (GtkToggleButton *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_info,
        gtk_toggle_button_get_active (widget));
    gtk_widget_grab_focus (GTK_WIDGET(widget));
}


static void
cb_m_prev (GtkWidget *widget, Dialog *d)
{
    step_back (d);
    show_current_card (d, FALSE);
}


static void
cb_b_prev (GtkWidget *widget, Dialog *d)
{
    cb_m_prev (widget, d);
    if (d->cur->prev)
        gtk_widget_grab_focus (widget);
    else
        gtk_widget_grab_focus (d->b_next);
}


static void
cb_m_next (GtkWidget *widget, Dialog *d)
{
    step_forward (d);
    show_current_card (d, FALSE);
}


static void
cb_b_next (GtkWidget *widget, Dialog *d)
{
    cb_m_next (widget, d);
    gtk_widget_grab_focus (widget);
}


static void
cb_m_unknown (GtkCheckMenuItem *tb, Dialog *d)
{
    gboolean marked_as_tested;


    toggle_ui_info (d, FALSE);

    if (!gtk_check_menu_item_get_active (tb) &&
        !gtk_check_menu_item_get_active (d->m_known)) {
        QUIZITEM(d->cur)->tested = FALSE;
        show_current_card (d, TRUE);
        return;
    }

    marked_as_tested = QUIZITEM(d->cur)->tested;

    QUIZITEM(d->cur)->known = FALSE;
    QUIZITEM(d->cur)->tested = TRUE;

    /* If the card was marked as untested, move on to the next card. */

    if (!marked_as_tested)
        step_forward (d);

    show_current_card (d, marked_as_tested);
}


static void
cb_b_unknown (GtkToggleButton *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_unknown,
        gtk_toggle_button_get_active (widget));
    gtk_widget_grab_focus (GTK_WIDGET(widget));
}


static void
cb_m_known (GtkCheckMenuItem *tb, Dialog *d)
{
    gboolean marked_as_tested;


    toggle_ui_info (d, FALSE);

    if (!gtk_check_menu_item_get_active (tb) &&
        !gtk_check_menu_item_get_active (d->m_unknown)) {
        QUIZITEM(d->cur)->tested = FALSE;
        show_current_card (d, TRUE);
        return;
    }

    marked_as_tested = QUIZITEM(d->cur)->tested;

    QUIZITEM(d->cur)->known = TRUE;
    QUIZITEM(d->cur)->tested = TRUE;

    /* If the card was marked as untested, move on to the next card. */

    if (!marked_as_tested)
        step_forward (d);

    show_current_card (d, marked_as_tested);
}


static void
cb_b_known (GtkToggleButton *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_known,
        gtk_toggle_button_get_active (widget));
    gtk_widget_grab_focus (GTK_WIDGET(widget));
}


static void
cb_m_flip (GtkCheckMenuItem *widget, Dialog *d)
{
    gint page;
    gboolean active;

    active = gtk_check_menu_item_get_active (widget);
    page = active ? BACK : FRONT;

    toggle_ui_flip (d, active);
    gtk_notebook_set_current_page (d->notebook, page);
    toggle_ui_info (d, FALSE);

    if (page == FRONT && (prefs_get_auto_listen (d->ig->prefs) & LISTEN_FRONT))
        audio_play_side (FRONT);
    else if (page == BACK && (prefs_get_auto_listen (d->ig->prefs) & LISTEN_BACK))
        audio_play_side (BACK);

}


static void
cb_b_flip (GtkToggleButton *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_flip,
        gtk_toggle_button_get_active (widget));
    gtk_widget_grab_focus (GTK_WIDGET(widget));
}


static void
cb_m_flag (GtkCheckMenuItem *widget, Dialog *d)
{
    gboolean state;
    Card *c;


    c = QCARD(d->cur);

    state = gtk_check_menu_item_get_active (widget);

    if (card_get_flagged (c) != state) {

        GtkTreeIter iter;
        GList *item;

        card_set_flagged (c, state);
        ig_file_changed (d->ig);

        /* If the card is listed in the main window, update that row. */

        item = app_window_find_treev_iter_with_data (d->ig->treev_card, &iter, c);
        if (item != NULL)
            app_window_refresh_card_row (d->ig, item);

        app_window_update_title (d->ig);
    }

    update_title ();
}


static void
cb_m_listen (GtkWidget *widget, Dialog *d)
{
    gint page;

    if (widget == d->m_listen_front)
        page = FRONT;
    else
        page = BACK;

    audio_play_side (page);
}


static void
cb_m_auto_listen (GtkCheckMenuItem *widget, Dialog *d)
{
    AutoListen listen = 0;

    if (gtk_check_menu_item_get_active (d->m_auto_listen_front))
        listen |= LISTEN_FRONT;

    if (gtk_check_menu_item_get_active (d->m_auto_listen_back))
        listen |= LISTEN_BACK;

    prefs_set_auto_listen (d->ig->prefs, listen);
}


void
dialog_quiz_set_sensitive (gboolean sensitive)
{
    /* This function (along with the global variable named 'dialog')
     * is a hack to prevent the user switching to a different card while a
     * sound is playing. Doing so would cause a crash. An alternative might
     * be to call audio_stop() inside step_forward() and step_back().
     */

    if (dialog == NULL)
        return;

    gtk_widget_set_sensitive (dialog->m_prev, sensitive);
    gtk_widget_set_sensitive (dialog->m_next, sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET(dialog->m_known), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET(dialog->m_unknown), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET(dialog->m_flip), sensitive);

    gtk_widget_set_sensitive (dialog->b_prev, sensitive);
    gtk_widget_set_sensitive (dialog->b_next, sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET(dialog->b_known), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET(dialog->b_unknown), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET(dialog->b_flip), sensitive);
}


static void
window_close (Dialog *d)
{
    gint w, h;
    GList *cur;
    QuizItem *last;


    audio_stop ();

    store_quiz_results (d, gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(d->chk_store)));

    last = d->last;
    card_free (last->card);

    for (cur = d->quizitems; cur != NULL; cur = cur->next)
        quizitem_free ((QuizItem*)cur->data);

    g_list_free (d->quizitems);

    if (d->cardview_selected)
        g_list_free (d->cardview_selected);

    gtk_window_get_size (GTK_WINDOW(d->window), &w, &h);
    prefs_set_quiz_size (d->ig->prefs, w, h);

    audio_free_all ();

    gtk_widget_destroy (d->window);
    g_free (d);

    dialog = NULL;
}


static void
cb_m_close (GtkWidget *widget, Dialog *d)
{
    window_close (d);
}


static void
cb_window_delete (GtkWidget *widget, GdkEvent *event, Dialog *d)
{
    window_close (d);
}


static void
toggle_ui_known (Dialog *d, gboolean active)
{
    g_signal_handlers_block_by_func (G_OBJECT(d->b_known),
        G_CALLBACK(cb_b_known), d);
    g_signal_handlers_block_by_func (G_OBJECT(d->m_known),
        G_CALLBACK(cb_m_known), d);

    gtk_toggle_button_set_active (d->b_known, active);
    gtk_check_menu_item_set_active (d->m_known, active);

    g_signal_handlers_unblock_by_func (G_OBJECT(d->b_known),
        G_CALLBACK(cb_b_known), d);
    g_signal_handlers_unblock_by_func (G_OBJECT(d->m_known),
        G_CALLBACK(cb_m_known), d);
}


static void
toggle_ui_unknown (Dialog *d, gboolean active)
{
    g_signal_handlers_block_by_func (G_OBJECT(d->b_unknown),
        G_CALLBACK(cb_b_unknown), d);
    g_signal_handlers_block_by_func (G_OBJECT(d->m_unknown),
        G_CALLBACK(cb_m_unknown), d);

    gtk_toggle_button_set_active (d->b_unknown, active);
    gtk_check_menu_item_set_active (d->m_unknown, active);

    g_signal_handlers_unblock_by_func (G_OBJECT(d->b_unknown),
        G_CALLBACK(cb_b_unknown), d);
    g_signal_handlers_unblock_by_func (G_OBJECT(d->m_unknown),
        G_CALLBACK(cb_m_unknown), d);
}


static void
toggle_ui_info (Dialog *d, gboolean active)
{
    g_signal_handlers_block_by_func (G_OBJECT(d->b_info),
        G_CALLBACK(cb_b_info), d);
    g_signal_handlers_block_by_func (G_OBJECT(d->m_info),
        G_CALLBACK(cb_m_info), d);

    gtk_toggle_button_set_active (d->b_info, active);
    gtk_check_menu_item_set_active (d->m_info, active);

    g_signal_handlers_unblock_by_func (G_OBJECT(d->b_info),
        G_CALLBACK(cb_b_info), d);
    g_signal_handlers_unblock_by_func (G_OBJECT(d->m_info),
        G_CALLBACK(cb_m_info), d);
}


static void
toggle_ui_flip (Dialog *d, gboolean active)
{
    g_signal_handlers_block_by_func (G_OBJECT(d->b_flip),
        G_CALLBACK(cb_b_flip), d);
    g_signal_handlers_block_by_func (G_OBJECT(d->m_flip),
        G_CALLBACK(cb_m_flip), d);

    gtk_toggle_button_set_active (d->b_flip, active);
    gtk_check_menu_item_set_active (d->m_flip, active);

    g_signal_handlers_unblock_by_func (G_OBJECT(d->b_flip),
        G_CALLBACK(cb_b_flip), d);
    g_signal_handlers_unblock_by_func (G_OBJECT(d->m_flip),
        G_CALLBACK(cb_m_flip), d);
}


static void
update_title (void)
{
    Card *c;
    gchar *title, *flag = "";

    if (!dialog)
        return;

    c = QCARD(dialog->cur);

    if (!CUR_IS_LAST(dialog) && card_get_flagged (c))
        flag = UNICHAR_FLAGGED;

    title = g_strdup_printf (_("Quiz %s"), flag);

    gtk_window_set_title (GTK_WINDOW(dialog->window), title);

    g_free (title);
}


void
dialog_quiz (Ignuit *ig, QuizMode quiz_mode)
{
    Dialog    *d;
    GtkWidget *label, *m_close, *b_close_answer_bar;
    GladeXML  *glade_xml;
    gchar     *glade_file;
    Card      *c;


    glade_file = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_QUIZ, TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_QUIZ);
        return;
    }

    dialog = d = g_new0 (Dialog, 1);

    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;

    d->quiz_mode = quiz_mode;

    d->cardview_selected = NULL;
 
    if (d->ig->quizinfo.card_selection == QUIZ_SELECTED_CARDS) {

        /* Get a list of selected items from the main window's card treeview. */

        d->cardview_selected = treev_get_selected_items (d->ig->treev_card);

    }

    d->quizitems = get_quizitems (d, ig->grand);

    c = card_new ();
    d->last = quizitem_new (c);
    d->quizitems = g_list_append (d->quizitems, d->last);
    d->cur = d->quizitems;

    card_set_style (c, CARD_STYLE_KEYWORDS_AND_CENTERED_TEXT);

    d->n = 1;
    d->n_quizitems = g_list_length (d->quizitems) - 1;

    /* Store start date and time. */
    date_today (&d->date_tested);
    d->time_tested = get_current_hour ();

    if (d->n_quizitems == 0) {

        switch (d->ig->quizinfo.card_selection) {
        case QUIZ_ALL_CARDS:
            /* GUI sensitivity should've prevented us from starting
             * a quiz in an empty file or empty category. */
            g_assert_not_reached ();
            break;
        case QUIZ_NEW_CARDS:
            card_set_front (c,
            _("No cards\n\nThere are no new cards to test.\nSee the <i>Quiz</i> menu for options."));
            break;
        case QUIZ_EXPIRED_CARDS:
            card_set_front (c,
            _("No cards\n\nThere are no expired cards to test.\nSee the <i>Quiz</i> menu for options."));
            break;
        case QUIZ_NEW_AND_EXPIRED_CARDS:
            card_set_front (c,
            _("No cards\n\nThere are no new or expired cards to test.\nSee the <i>Quiz</i> menu for options."));
            break;
        case QUIZ_FLAGGED_CARDS:
            card_set_front (c,
            _("No cards\n\nThere are no cards flagged.\nSee the <i>Quiz</i> menu for options."));
            break;
        case QUIZ_SELECTED_CARDS:
            card_set_front (c,
            _("No cards\n\nThere are no cards selected.\nSee the <i>Quiz</i> menu for options."));
            break;
        default:
            g_assert_not_reached ();
            break;
        }

    }
    else {
        card_set_front (c,
            _("End of quiz\n\nUse the buttons below to review your choices. Only cards marked as <i>Known</i> or <i>Unknown</i> are recorded. Nothing is recorded until the quiz window closes."));
    }


    d->window = glade_xml_get_widget (glade_xml, "dialog");

    gtk_window_set_default_size (GTK_WINDOW(d->window),
        prefs_get_quiz_width (d->ig->prefs),
        prefs_get_quiz_height (d->ig->prefs));

    d->notebook = GTK_NOTEBOOK(glade_xml_get_widget (glade_xml, "notebook"));


    /* Front textview */

    d->textview[FRONT] = GTK_TEXT_VIEW(glade_xml_get_widget (glade_xml,
        "tv_front"));
    d->textbuf[FRONT] = gtk_text_view_get_buffer (d->textview[FRONT]);
    textbuf_create_tags (d->textbuf[FRONT]);
    textview_set_properties (d->textview[FRONT], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_QUIZ);

    /* Back textview */

    d->textview[BACK] = GTK_TEXT_VIEW(glade_xml_get_widget (glade_xml,
        "tv_back"));
    d->textbuf[BACK] = gtk_text_view_get_buffer (d->textview[BACK]);
    textbuf_create_tags (d->textbuf[BACK]);
    textview_set_properties (d->textview[BACK], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_QUIZ);

    /* Details textview */

    d->textview[INFO] = GTK_TEXT_VIEW(glade_xml_get_widget (glade_xml,
        "tv_info"));
    d->textbuf[INFO] = gtk_text_view_get_buffer (d->textview[INFO]);
    textbuf_create_tags (d->textbuf[INFO]);
    textview_set_properties (d->textview[INFO], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_INFO);


    d->m_prev = glade_xml_get_widget (glade_xml, "m_prev");
    d->m_next = glade_xml_get_widget (glade_xml, "m_next");
    d->m_unknown = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_unknown");
    d->m_known = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_known");
    d->m_flip = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_flip");
    d->m_info = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_info");
    d->m_answer_bar = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_answer_bar");
    d->m_listen_front = (GtkWidget*)glade_xml_get_widget
        (glade_xml, "m_listen_front");
    d->m_listen_back = (GtkWidget*)glade_xml_get_widget
        (glade_xml, "m_listen_back");
    d->m_auto_listen_front = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_auto_listen_front");
    d->m_auto_listen_back = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_auto_listen_back");
    d->m_flag = (GtkCheckMenuItem*)glade_xml_get_widget
        (glade_xml, "m_flag");
    m_close = glade_xml_get_widget (glade_xml, "m_close");


    d->hbox_answer_bar = glade_xml_get_widget (glade_xml, "hbox_answer_bar");
    d->entry_answer_bar = glade_xml_get_widget (glade_xml, "entry_answer_bar");
    d->b_clear_answer_bar = glade_xml_get_widget (glade_xml, "b_clear_answer_bar");
    b_close_answer_bar = glade_xml_get_widget (glade_xml, "b_close_answer_bar");

    d->b_info = (GtkToggleButton*)glade_xml_get_widget (glade_xml, "b_info");
    d->b_prev = glade_xml_get_widget (glade_xml, "b_prev");
    d->b_next = glade_xml_get_widget (glade_xml, "b_next");
    d->chk_store = glade_xml_get_widget (glade_xml, "chk_store");
    d->b_unknown = (GtkToggleButton*)glade_xml_get_widget
        (glade_xml, "b_unknown");
    d->b_known = (GtkToggleButton*)glade_xml_get_widget
        (glade_xml, "b_known");
    d->b_flip = (GtkToggleButton*)glade_xml_get_widget
        (glade_xml, "b_flip");

    label = glade_xml_get_widget (glade_xml, "label_answer_bar");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_answer_bar);

    d->statusbar = (GtkStatusbar*)glade_xml_get_widget
        (glade_xml, "statusbar");
    d->sbcid = gtk_statusbar_get_context_id (d->statusbar, "quiz");

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(d->chk_store),
        d->quiz_mode == QUIZ_MODE_NORMAL);

    g_signal_connect (G_OBJECT(d->window), "delete-event",
        G_CALLBACK(cb_window_delete), d);

    g_signal_connect (G_OBJECT(d->m_prev), "activate",
        G_CALLBACK(cb_m_prev), d);
    g_signal_connect (G_OBJECT(d->m_next), "activate",
        G_CALLBACK(cb_m_next), d);
    g_signal_connect (G_OBJECT(d->m_unknown), "toggled",
        G_CALLBACK(cb_m_unknown), d);
    g_signal_connect (G_OBJECT(d->m_known), "toggled",
        G_CALLBACK(cb_m_known), d);

    g_signal_connect (G_OBJECT(d->m_flip), "toggled",
        G_CALLBACK(cb_m_flip), d);

    g_signal_connect (G_OBJECT(d->m_listen_front), "activate",
        G_CALLBACK(cb_m_listen), d);
    g_signal_connect (G_OBJECT(d->m_listen_back), "activate",
        G_CALLBACK(cb_m_listen), d);
    g_signal_connect (G_OBJECT(d->m_auto_listen_front), "toggled",
        G_CALLBACK(cb_m_auto_listen), d);
    g_signal_connect (G_OBJECT(d->m_auto_listen_back), "toggled",
        G_CALLBACK(cb_m_auto_listen), d);

    g_signal_connect (G_OBJECT(d->m_flag), "toggled",
        G_CALLBACK(cb_m_flag), d);

    g_signal_connect (G_OBJECT(d->m_info), "toggled",
        G_CALLBACK(cb_m_info), d);

    g_signal_connect (G_OBJECT(d->m_answer_bar), "toggled",
        G_CALLBACK(cb_m_answer_bar), d);

    g_signal_connect (G_OBJECT(m_close), "activate",
        G_CALLBACK(cb_m_close), d);

    g_signal_connect (G_OBJECT(b_close_answer_bar), "clicked",
        G_CALLBACK(cb_b_close_answer_bar), d);
    g_signal_connect (G_OBJECT(d->b_clear_answer_bar), "clicked",
        G_CALLBACK(cb_b_clear_answer_bar), d);
    g_signal_connect (G_OBJECT(d->entry_answer_bar), "activate",
        G_CALLBACK(cb_answer_bar_activate), d);

    g_signal_connect (G_OBJECT(d->b_info), "toggled",
        G_CALLBACK(cb_b_info), d);
    g_signal_connect (G_OBJECT(d->b_prev), "clicked",
        G_CALLBACK(cb_b_prev), d);
    g_signal_connect (G_OBJECT(d->b_next), "clicked",
        G_CALLBACK(cb_b_next), d);
    g_signal_connect (G_OBJECT(d->b_unknown), "toggled",
        G_CALLBACK(cb_b_unknown), d);
    g_signal_connect (G_OBJECT(d->b_known), "toggled",
        G_CALLBACK(cb_b_known), d);
    g_signal_connect (G_OBJECT(d->b_flip), "toggled",
        G_CALLBACK(cb_b_flip), d);

    gtk_window_set_transient_for (GTK_WINDOW(d->window),
        GTK_WINDOW(ig->app));
    gtk_window_set_modal (GTK_WINDOW(d->window), TRUE);


    g_signal_handlers_block_by_func (G_OBJECT(d->m_auto_listen_front),
        G_CALLBACK(cb_m_auto_listen), d);
    g_signal_handlers_block_by_func (G_OBJECT(d->m_auto_listen_back),
        G_CALLBACK(cb_m_auto_listen), d);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(d->m_answer_bar),
        prefs_get_quiz_answer_bar_visible (d->ig->prefs));

    if (!prefs_get_quiz_answer_bar_visible (d->ig->prefs))
        gtk_widget_hide (d->hbox_answer_bar);

    gtk_check_menu_item_set_active (d->m_auto_listen_front,
        prefs_get_auto_listen (d->ig->prefs) & LISTEN_FRONT);
    gtk_check_menu_item_set_active (d->m_auto_listen_back,
        prefs_get_auto_listen (d->ig->prefs) & LISTEN_BACK);

    g_signal_handlers_unblock_by_func (G_OBJECT(d->m_auto_listen_front),
        G_CALLBACK(cb_m_auto_listen), d);
    g_signal_handlers_unblock_by_func (G_OBJECT(d->m_auto_listen_back),
        G_CALLBACK(cb_m_auto_listen), d);


    gtk_widget_show (d->window);

    g_object_unref (G_OBJECT(glade_xml));

    show_current_card (d, FALSE);
    set_interface_sensitivity (d);
}

