/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
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


#include <config.h>
#include <gnome.h>
#include <glib/gi18n.h>
#include <glade/glade.h>

#include "main.h"
#include "prefs.h"
#include "file.h"
#include "dialog-editor.h"
#include "dialog-tagger.h"
#include "app-window.h"
#include "textview.h"


#if 0
#define DEBUG_UNDO_1
#endif
#if 0
#define DEBUG_UNDO_2
#endif


#define ACTION(listitem) ((Action*)listitem->data)


enum {
    ACTION_NONE = 0,
    ACTION_INSERT,
    ACTION_DELETE,
    ACTION_BACKSPACE
};


typedef struct _Action Action;

struct _Action {

    gint    type;
    gint    start;  /* UTF-8 characters */
    gint    length; /* UTF-8 characters */
    GString *gstr;

};

typedef struct {

    Ignuit              *ig;

    GtkWidget           *window;

    GtkNotebook         *notebook;
    GtkTextView         *textview[3];
    GtkTextBuffer       *textbuf[3];

    GtkWidget           *hbox_tag_bar;
    GtkWidget           *entry_tag_bar;
    GtkWidget           *b_clear_tag_bar;

    GtkWidget           *b_add;
    GtkWidget           *b_remove;
    GtkWidget           *b_prev;
    GtkWidget           *b_next;
    GtkToggleButton     *b_flip;
    GtkToggleButton     *b_info;

    GtkWidget           *m_add;
    GtkWidget           *m_remove;
    GtkWidget           *m_prev;
    GtkWidget           *m_next;
    GtkCheckMenuItem    *m_flip;
    GtkCheckMenuItem    *m_tag_bar;
    GtkCheckMenuItem    *m_flag;
    GtkCheckMenuItem    *m_info;
    GtkWidget           *m_switch_sides;
    GtkWidget           *m_reset_stats;

    GtkWidget           *m_undo;
    GtkWidget           *m_redo;
    GtkWidget           *m_insert_image;
    GtkWidget           *m_insert_sound;

    GtkWidget           *m_cut;
    GtkWidget           *m_copy;
    GtkWidget           *m_paste;
    GtkWidget           *m_delete;

    gchar               *s_tag_bar;

    GList               *undo[2];
    GList               *redo[2];

    guint               keyval;

    GtkStatusbar        *statusbar;
    guint               sbcid;

} Dialog;


static void cb_textbuf_modified (GtkTextBuffer *buf, Dialog *d);
static void cb_m_flag (GtkCheckMenuItem *widget, Dialog *d);
static void cb_b_info (GtkToggleButton *tb, Dialog *d);
static void toggle_ui_info (Dialog *d, gboolean active);
static void toggle_ui_flip (Dialog *d, gboolean active);
static void update_undo_sensitivity (Dialog *d);
static void update_insert_sensitivity (Dialog *d);


static Dialog *dialog = NULL;


static gchar*
prompt_media_filename (Dialog *d)
{
    GtkWidget *prompt;
    gint result;
    gchar *fname;

    prompt = gtk_file_chooser_dialog_new (_("Open"),
        GTK_WINDOW(d->window), GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response (GTK_DIALOG(prompt), GTK_RESPONSE_ACCEPT);

    if (prefs_get_workdir (d->ig->prefs)) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(prompt),
            prefs_get_workdir (d->ig->prefs));
    }

    result = gtk_dialog_run (GTK_DIALOG(prompt));
    fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(prompt));
    gtk_widget_destroy (prompt);

    if (result == GTK_RESPONSE_ACCEPT)
        return fname;

    g_free (fname);

    return NULL;
}


static Action*
action_new (gint action_type, gint start, gint length, const gchar *str)
{
    Action *a;

    a = g_new (Action, 1);
    a->type = action_type;
    a->start = start;
    a->length = length;
    a->gstr = g_string_new (str);

    return a;
}


static void
action_free (Action *a)
{
    if (a->gstr != NULL)
        g_string_free (a->gstr, TRUE);

    g_free (a);
}


static void
action_list_free (GList *list)
{
    GList *item;

    for (item = list; item != NULL; item = item->next)
        action_free (item->data);

    g_list_free (list);
}


static void
reset_undo_redo (Dialog *d, gint p)
{
    action_list_free (d->undo[p]);
    d->undo[p] = NULL;

    action_list_free (d->redo[p]);
    d->redo[p] = NULL;
}


static void
cb_textbuf_end_user_action (GtkTextBuffer *textbuf, Dialog *d)
{
    update_undo_sensitivity (d);
}


static void
action_add (gint a_type, gint start, gint length, Dialog *d)
{
    GtkTextIter i_start, i_end;
    Action *a, *a_prev;
    gchar *str;
    gboolean continuation;
    gint p;


    p = gtk_notebook_get_current_page (d->notebook);

    gtk_text_buffer_get_iter_at_offset (d->textbuf[p], &i_start, start);
    gtk_text_buffer_get_iter_at_offset (d->textbuf[p], &i_end, start + length);
    str = gtk_text_buffer_get_text (d->textbuf[p], &i_start, &i_end, FALSE);

#ifdef DEBUG_UNDO_1
    g_printerr ("%d+%d (%d) <%s>\n",
        start,
        start + length,
        length,
        str);
#endif

    a_prev = d->undo[p] ? ACTION(d->undo[p]) : NULL;

    continuation = a_prev != NULL &&
        start == a_prev->start + a_prev->length &&
        d->keyval != GDK_space &&
        d->keyval != GDK_Tab &&
        d->keyval != GDK_Return;

    if (a_prev == NULL || a_prev->type != a_type ||
        length > 1 || !continuation) {

        /* Begin a new action. */

        a = action_new (a_type, start, length, str);
        d->undo[p] = g_list_prepend (d->undo[p], a);

    }
    else {

        /* Continue the current action. */

        a = a_prev;
        a->length = a->length + length;

        switch (a->type) {
        case ACTION_INSERT:
            g_string_append (a->gstr, str);
            break;
        case ACTION_DELETE:
        case ACTION_BACKSPACE:
            g_string_prepend (a->gstr, str);
            break;
        default:
            g_assert_not_reached ();
            break;
        }

        action_list_free (d->redo[p]);
        d->redo[p] = NULL;

    }

#ifdef DEBUG_UNDO_2
    g_printerr ("%s (undo %d, redo %d): [%s]\n",
        a_type == ACTION_INSERT ? "INS" : "DEL/BS",
        g_list_length (d->undo[p]),
        g_list_length (d->redo[p]),
        a->gstr->str);
#endif
}


static void
cb_textbuf_insert_text (GtkTextBuffer *textbuf, GtkTextIter *iter,
    gchar *text, gint len, Dialog *d)
{
    gint start, end, length;

    length = g_utf8_strlen (text, -1);
    end = gtk_text_iter_get_offset (iter);
    start = end - length;

    action_add (ACTION_INSERT, start, length, d);
}


static void
cb_textbuf_delete_range (GtkTextBuffer *textbuf,
    GtkTextIter *i_start, GtkTextIter *i_end, Dialog *d)
{
    gint start, end, length, action;

    start = gtk_text_iter_get_offset (i_start);
    end = gtk_text_iter_get_offset (i_end);
    length = end - start;

    if (d->keyval == GDK_BackSpace)
        action = ACTION_BACKSPACE;
    else
        action = ACTION_DELETE;

    action_add (action, start, length, d);
}


static void
textbuf_block_signals (Dialog *d, gint p)
{
    g_signal_handlers_block_by_func (G_OBJECT(d->textbuf[p]),
        G_CALLBACK(cb_textbuf_modified), d);

    g_signal_handlers_block_by_func (G_OBJECT(d->textbuf[p]),
        G_CALLBACK(cb_textbuf_insert_text), d);
    g_signal_handlers_block_by_func (G_OBJECT(d->textbuf[p]),
        G_CALLBACK(cb_textbuf_delete_range), d);
}


static void
textbuf_unblock_signals (Dialog *d, gint p)
{
    g_signal_handlers_unblock_by_func (G_OBJECT(d->textbuf[p]),
        G_CALLBACK(cb_textbuf_modified), d);

    g_signal_handlers_unblock_by_func (G_OBJECT(d->textbuf[p]),
        G_CALLBACK(cb_textbuf_insert_text), d);
    g_signal_handlers_unblock_by_func (G_OBJECT(d->textbuf[p]),
        G_CALLBACK(cb_textbuf_delete_range), d);
}


static gboolean
cb_textview_key_press_event (GtkWidget *textview, GdkEventKey *event, Dialog *d)
{
    switch (event->keyval) {
    case GDK_space:
    case GDK_Return:
    case GDK_Tab:
    case GDK_BackSpace:
    case GDK_Delete:
        d->keyval = event->keyval;
        break;
    default:
        d->keyval = 0;
        break;
    }

    return FALSE;
}


static gboolean
cb_window_key_press_event (GtkWindow *w, GdkEventKey *event, Dialog *d)
{
    gboolean handled;

#if 0
    if (event->state & GDK_CONTROL_MASK && event->keyval == GDK_s) {
        /* XXX: Save the file. */
        return TRUE;
    }
#endif

    /* The following is necessary to prevent copy/paste key presses intended
     * for the focused tag bar from ending up in the textview.
     * (This solution found in gedit 2.28.0, gedit-window.c) */

    handled = gtk_window_propagate_key_event (w, event);

    if (!handled)
        handled = gtk_window_activate_key (w, event);

    return handled;
}


static void
cb_m_undo (GtkWidget *w, Dialog *d)
{
    GtkTextIter i_start, i_end, i_cursor;
    Action *a;
    gint p, end;
#ifdef DEBUG_UNDO_2
    const gchar *msg;
#endif


    p = gtk_notebook_get_current_page (d->notebook);

    if (d->undo[p] == NULL)
        return;

    textbuf_block_signals (d, p);

    a = ACTION(d->undo[p]);

    end = a->start + a->length;

    gtk_text_buffer_get_iter_at_offset (d->textbuf[p], &i_start, a->start);

    switch (a->type) {
    case ACTION_INSERT:

#ifdef DEBUG_UNDO_2
        msg = "undo INS";
#endif
        gtk_text_buffer_get_iter_at_offset (d->textbuf[p], &i_end, end);
        gtk_text_buffer_delete (d->textbuf[p], &i_start, &i_end);
        textbuf_place_cursor (d->textbuf[p], &i_cursor, a->start);
        break;

    case ACTION_DELETE:

#ifdef DEBUG_UNDO_2
        msg = "undo DEL";
#endif
        gtk_text_buffer_insert (d->textbuf[p], &i_start, a->gstr->str, -1);
        textbuf_place_cursor (d->textbuf[p], &i_cursor, a->start);
        break;

    case ACTION_BACKSPACE:

#ifdef DEBUG_UNDO_2
        msg = "undo BS";
#endif
        gtk_text_buffer_insert (d->textbuf[p], &i_start, a->gstr->str, -1);
        textbuf_place_cursor (d->textbuf[p], &i_cursor, a->start + a->length);
        break;

    default:
        g_assert_not_reached ();
        break;
    }

    gtk_text_view_scroll_to_iter (d->textview[p],
        &i_cursor, 0.1, FALSE, 0.5, 0.5);

    d->undo[p] = g_list_delete_link (d->undo[p], d->undo[p]);
    d->redo[p] = g_list_prepend (d->redo[p], a);

#ifdef DEBUG_UNDO_2
    g_printerr ("%s (undo %d, redo %d): [%s]\n", msg,
        g_list_length (d->undo[p]),
        g_list_length (d->redo[p]), a->gstr->str);
#endif

    textbuf_unblock_signals (d, p);

    update_undo_sensitivity (d);
}


static void
cb_m_redo (GtkWidget *w, Dialog *d)
{
    GtkTextIter i_start, i_end, i_cursor;
    Action *a;
    gint p, end;
#ifdef DEBUG_UNDO_2
    const gchar *msg;
#endif


    p = gtk_notebook_get_current_page (d->notebook);

    if (d->redo[p] == NULL)
        return;

    textbuf_block_signals (d, p);

    a = ACTION(d->redo[p]);

    end = a->start + a->length;
    gtk_text_buffer_get_iter_at_offset (d->textbuf[p], &i_start, a->start);

    switch (a->type) {
    case ACTION_INSERT:

#ifdef DEBUG_UNDO_2
        msg = "redo INS";
#endif
        gtk_text_buffer_insert (d->textbuf[p], &i_start, a->gstr->str, -1);
        textbuf_place_cursor (d->textbuf[p], &i_cursor, end);
        break;

    case ACTION_DELETE:
    case ACTION_BACKSPACE:

#ifdef DEBUG_UNDO_2
        msg = "redo DEL/BS";
#endif
        gtk_text_buffer_get_iter_at_offset (d->textbuf[p], &i_end, end);
        gtk_text_buffer_delete (d->textbuf[p], &i_start, &i_end);
        textbuf_place_cursor (d->textbuf[p], &i_cursor, a->start);
        break;

    default:
        g_assert_not_reached ();
        break;
    }

    gtk_text_view_scroll_to_iter (d->textview[p],
        &i_cursor, 0.1, FALSE, 0.5, 0.5);

    d->redo[p] = g_list_delete_link (d->redo[p], d->redo[p]);
    d->undo[p] = g_list_prepend (d->undo[p], a);

#ifdef DEBUG_UNDO_2
    g_printerr ("%s (undo %d, redo %d): [%s]\n", msg,
        g_list_length (d->undo[p]),
        g_list_length (d->redo[p]), a->gstr->str);
#endif

    textbuf_unblock_signals (d, p);

    update_undo_sensitivity (d);
}


static void
update_editor_title (void)
{
    GList *item;
    gchar *title, *flag;

    if (dialog == NULL)
        return;

    if ((item = file_get_current_item (dialog->ig->file)) != NULL) {
        flag = card_get_flagged (CARD(item)) ? UNICHAR_FLAGGED : "";
    }
    else {
        flag = "";
    }

    title = g_strdup_printf (_("Card Editor %s"), flag);

    gtk_window_set_title (GTK_WINDOW(dialog->window), title);

    g_free (title);
}


static void
set_statusbar_message (Dialog *d, const gchar *text)
{
    gtk_statusbar_pop (d->statusbar, d->sbcid);
    gtk_statusbar_push (d->statusbar, d->sbcid, text);
}


static void
update_editor_statusbar (Dialog *d)
{
    GList *item;
    gchar *s;


    item = file_get_current_item (d->ig->file);

    if (file_get_current_category (d->ig->file) == NULL) {
        s = g_strdup (_("No category selected"));
    }
    else if (!item) {
        if (file_current_category_is_trash (d->ig->file)) {
            s = g_strdup (_("No cards in Trash"));
        }
        else {
            s = g_strdup (_("No cards"));
        }
    }
    else if (card_get_n_tests (CARD(item))) {
        Card *c = CARD(item);
        s = g_strdup_printf (_("Box %d, Success %.0f%%. Category: %s"),
            card_get_group (c),
            card_get_score (c),
            category_get_title (card_get_category (c)));
    }
    else {
        Card *c = CARD(item);
        s = g_strdup_printf (_("Never tested. Category: %s"),
            category_get_title (card_get_category (c)));
    }

    set_statusbar_message (d, s);
    g_free (s);
}


static void
update_undo_sensitivity (Dialog *d)
{
    gboolean can_undo, can_redo;
    gint p;

    p = gtk_notebook_get_current_page (d->notebook);

    can_undo = (p == FRONT || p == BACK) && (d->undo[p] != NULL);
    can_redo = (p == FRONT || p == BACK) && (d->redo[p] != NULL);

    gtk_widget_set_sensitive (d->m_undo, can_undo);
    gtk_widget_set_sensitive (d->m_redo, can_redo);
}


static void
update_insert_sensitivity (Dialog *d)
{
    gboolean can_insert;
    GList *item;
    gint p;

    p = gtk_notebook_get_current_page (d->notebook);
    item = file_get_current_item (d->ig->file);

    can_insert = (item != NULL) && (p == FRONT || p == BACK);

    gtk_widget_set_sensitive (d->m_insert_image, can_insert);
    gtk_widget_set_sensitive (d->m_insert_sound, can_insert);
}


static void
update_editor_sensitivity (Dialog *d)
{
    GdkColor *color;
    Category *cat;
    gboolean have_card, can_move, can_add;


    /* For use when switching between cards. */

    cat = file_get_current_category (d->ig->file);

    can_add = !(cat == NULL
        || file_category_is_search (d->ig->file, cat)
        || file_category_is_trash (d->ig->file, cat)
        || ig_category_is_clipboard (d->ig, cat));

    can_move = (cat != NULL && category_get_n_cards (cat) > 1);

    gtk_widget_set_sensitive (d->b_prev, can_move);
    gtk_widget_set_sensitive (d->b_next, can_move);
    gtk_widget_set_sensitive (d->m_prev, can_move);
    gtk_widget_set_sensitive (d->m_next, can_move);

    have_card = (file_get_current_item (d->ig->file) != NULL);

    gtk_widget_set_sensitive (d->b_add, can_add);
    gtk_widget_set_sensitive (d->b_remove, have_card);
    gtk_widget_set_sensitive (GTK_WIDGET(d->b_flip), have_card);
    gtk_widget_set_sensitive (GTK_WIDGET(d->b_info), have_card);
    gtk_widget_set_sensitive (d->m_add, can_add);
    gtk_widget_set_sensitive (d->m_remove, have_card);
    gtk_widget_set_sensitive (GTK_WIDGET(d->m_flip), have_card);
    gtk_widget_set_sensitive (GTK_WIDGET(d->m_flag), have_card);
    gtk_widget_set_sensitive (GTK_WIDGET(d->m_info), have_card);
    gtk_widget_set_sensitive (d->m_switch_sides, have_card);
    gtk_widget_set_sensitive (d->m_reset_stats, have_card);

    gtk_widget_set_sensitive (d->entry_tag_bar, have_card);
    gtk_widget_set_sensitive (d->b_clear_tag_bar, have_card);

    update_undo_sensitivity (d);
    update_insert_sensitivity (d);

    gtk_text_view_set_editable (d->textview[FRONT], have_card);
    gtk_text_view_set_cursor_visible (d->textview[FRONT], have_card);

    color = have_card ? prefs_get_color (d->ig->prefs, COLOR_CARD_BG) :
        prefs_get_color (d->ig->prefs, COLOR_CARD_BG_END);

    gtk_widget_modify_base (GTK_WIDGET(d->textview[FRONT]),
        GTK_STATE_NORMAL, color);
}


static void
update_main_window_sensitivity (Dialog *d)
{
    gboolean have_current, file_not_empty;

    /* For use after adding or removing a card. */

    have_current = (file_get_current_item (d->ig->file) != NULL);
    file_not_empty = (file_get_n_cards (d->ig->file) > 0);

    gtk_widget_set_sensitive (d->ig->m_find, file_not_empty);
    gtk_widget_set_sensitive (d->ig->t_find, file_not_empty);
    gtk_widget_set_sensitive (d->ig->m_find_flagged, file_not_empty);
    gtk_widget_set_sensitive (d->ig->m_find_all, file_not_empty);
    gtk_widget_set_sensitive (d->ig->m_select_all, have_current);
    gtk_widget_set_sensitive (d->ig->m_edit_tags, have_current);
    gtk_widget_set_sensitive (d->ig->m_flag, have_current);
    gtk_widget_set_sensitive (d->ig->m_switch_sides, have_current);
    gtk_widget_set_sensitive (d->ig->m_reset_stats, have_current);
    gtk_widget_set_sensitive (d->ig->m_start_quiz, file_not_empty);
    gtk_widget_set_sensitive (d->ig->m_start_drill, file_not_empty);
    gtk_widget_set_sensitive (d->ig->t_start_quiz, file_not_empty);

    gtk_widget_set_sensitive (d->ig->m_card_popup_select_all, have_current);
    gtk_widget_set_sensitive (d->ig->m_card_popup_edit_tags, have_current);
    gtk_widget_set_sensitive (d->ig->m_card_popup_flag, have_current);
    gtk_widget_set_sensitive (d->ig->m_card_popup_switch_sides, have_current);
    gtk_widget_set_sensitive (d->ig->m_card_popup_reset_stats, have_current);

    gtk_widget_set_sensitive (d->ig->m_view_trash,
        file_get_trash (d->ig->file) != NULL);
}


static void
display_text (GtkTextBuffer *textbuf, const gchar *text)
{
    if (text == NULL)
        text = "";

    textbuf_block_signals (dialog, FRONT);
    textbuf_block_signals (dialog, BACK);

    gtk_text_buffer_set_text (textbuf, text, -1);
    gtk_text_buffer_set_modified (textbuf, FALSE);

    textbuf_unblock_signals (dialog, FRONT);
    textbuf_unblock_signals (dialog, BACK);
}


static void
dialog_blank (void)
{
    toggle_ui_flip (dialog, FALSE);
    toggle_ui_info (dialog, FALSE);

    reset_undo_redo (dialog, FRONT);
    reset_undo_redo (dialog, BACK);

    display_text (dialog->textbuf[FRONT], NULL);

    gtk_notebook_set_current_page (dialog->notebook, FRONT);

    gtk_entry_set_text (GTK_ENTRY(dialog->entry_tag_bar), "");

    update_editor_sensitivity (dialog);
    update_editor_title ();
}


static void
show_current_card (Dialog *d, EdTweak tweak)
{
    GList *item;
    Card *c;


    if ((item = file_get_current_item (d->ig->file)) == NULL)
        return;

    c = CARD(item);

    if (tweak & ED_TWEAK_TAG_BAR) {

        g_free (d->s_tag_bar);
        d->s_tag_bar = card_get_tags_as_string (c);
        gtk_entry_set_text (GTK_ENTRY(d->entry_tag_bar), d->s_tag_bar);

    }

    if (tweak & ED_TWEAK_TEXTVIEW) {

        reset_undo_redo (d, FRONT);
        reset_undo_redo (d, BACK);

        display_text (d->textbuf[FRONT], card_get_front (c));
        display_text (d->textbuf[BACK], card_get_back (c));

    }

    if (tweak & ED_TWEAK_INFO) {

        CardStyle style;
        Card *info;

        info = card_get_details (c);
        style = card_get_style (info);

        gtk_text_view_set_justification (GTK_TEXT_VIEW(d->textview[INFO]),
            card_style_get_justification (style));

        textview_display_with_markup (d->ig, d->textview[INFO],
            card_get_front (info), style, INFO);

        card_free (info);

        update_editor_statusbar (d);
    }

    if (tweak & ED_TWEAK_UI) {

        update_editor_sensitivity (d);

        g_signal_handlers_block_by_func (G_OBJECT(d->m_flag),
            G_CALLBACK(cb_m_flag), d);
        gtk_check_menu_item_set_active (d->m_flag, card_get_flagged (c));
        g_signal_handlers_unblock_by_func (G_OBJECT(d->m_flag),
            G_CALLBACK(cb_m_flag), d);

    }

    if (tweak & ED_TWEAK_TITLE) {

        update_editor_title ();

    }
}


static gchar*
textview_get_changes (GtkTextView *textview)
{
    GtkTextIter start, end;
    GtkTextBuffer *textbuf;


    textbuf = gtk_text_view_get_buffer (textview);

    /* If no change, return NULL. */
    if (!gtk_text_buffer_get_modified (textbuf))
        return NULL;

    gtk_text_buffer_get_bounds (textbuf, &start, &end);
    return gtk_text_buffer_get_text (textbuf, &start, &end, FALSE);
}


static gboolean
textview_check_changed (Dialog *d)
{
    GtkTextBuffer *textbuf;
    GList *item;
    Card *c;
    gboolean changed = FALSE;
    gchar *text;
    const gchar *s;


    if ((item = file_get_current_item (d->ig->file)) == NULL)
        return FALSE;

    c = CARD(item);

    s = gtk_entry_get_text (GTK_ENTRY(d->entry_tag_bar));
    if (strcmp (d->s_tag_bar, s) != 0)
        changed = file_card_set_tags_from_string (d->ig->file, c, s);

    if ((text = textview_get_changes (d->textview[FRONT])) != NULL) {
        changed = TRUE;
        card_set_front (c, text);
        g_free (text);
    }

    if ((text = textview_get_changes (d->textview[BACK])) != NULL) {
        changed = TRUE;
        card_set_back (c, text);
        g_free (text);
    }

    if (changed) {
        ig_file_changed (d->ig);
        app_window_refresh_card_row (d->ig, item);
    }

    /* In case called from app-window.c cb_m_save, reset modified state: */
    textbuf = gtk_text_view_get_buffer (d->textview[FRONT]);
    gtk_text_buffer_set_modified (textbuf, FALSE);
    textbuf = gtk_text_view_get_buffer (d->textview[BACK]);
    gtk_text_buffer_set_modified (textbuf, FALSE);

    return changed;
}


static void
window_close (Dialog *d)
{
    gint w, h;


    textview_check_changed (d);

    gtk_window_get_size (GTK_WINDOW(d->window), &w, &h);
    prefs_set_quiz_size (d->ig->prefs, w, h);

    gtk_widget_destroy (d->window);

    reset_undo_redo (d, FRONT); /* Free undo/redo lists. */
    reset_undo_redo (d, BACK);

    g_free (d->s_tag_bar);

    /* In case tags were removed: */
    file_delete_unused_tags (d->ig->file);

    g_free (d);
    dialog = NULL;
}


static void
cb_m_add (GtkWidget *widget, Dialog *d)
{
    GtkTreeIter iter;
    GList       *item;
    Category    *cat;
    Card        *c;


    /* Make a new blank card, add it to the currently selected category,
     * and set it up for editing. */

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(d->b_info), FALSE);

    textview_check_changed (d);

    cat = file_get_current_category (d->ig->file);

    c = card_new ();
    card_set_front (c, "");
    card_set_back (c, "");
    file_add_card (d->ig->file, cat, c);
    ig_file_changed (d->ig);

    item = category_get_cards (cat);
    file_set_current_item (d->ig->file, item);

    app_window_add_card_iter (d->ig, &iter, item);
    app_window_refresh_category_row (d->ig, cat);

    show_current_card (d, ED_TWEAK_ALL);

    gtk_toggle_button_set_active (d->b_flip, FALSE);

    app_window_update_appbar (d->ig);
    update_main_window_sensitivity (d);
}


static void
cb_m_remove (GtkWidget *widget, Dialog *d)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GList *item, *next;
    Category *cat_cur, *cat_card;
    Card *c;


    textview_check_changed (d);

    /* Remove the current card from the main window's TreeView. */

    c = file_get_current_card (d->ig->file);
    item = app_window_find_treev_iter_with_data
        (d->ig->treev_card, &iter, c);

    model = gtk_tree_view_get_model (d->ig->treev_card);
    gtk_list_store_remove (GTK_LIST_STORE(model), &iter);


    /* Move the card to the trash, and select the next card. */

    next = item->next ? item->next : item->prev;

    file_set_current_item (d->ig->file, next);

    cat_cur = file_get_current_category (d->ig->file);
    cat_card = card_get_category (c);

    if (file_category_is_search (d->ig->file, cat_cur)) {
        file_remove_card (d->ig->file, c);
        category_remove_card (cat_cur, c);
    }
    else {
        file_remove_card (d->ig->file, c);
    }

    if (file_category_is_trash (d->ig->file, cat_cur) || card_is_blank (c)) {
        /* Might as well delete it immediately. */
        card_free (c);
    }
    else {
        /* Move it to the trash. */
        file_add_trash (d->ig->file, c);
    }

    if (!file_category_is_trash (d->ig->file, cat_cur))
        app_window_refresh_category_row (d->ig, cat_card);

    ig_file_changed (d->ig);


    /* Update main window controls and appbar. */

    app_window_update_appbar (d->ig);
    update_main_window_sensitivity (d);


    /* Update the editor window. */

    if (next != NULL) {

        app_window_select_card (d->ig, next);

        if (!prefs_get_sticky_flips (d->ig->prefs))
            gtk_toggle_button_set_active (d->b_flip, FALSE);

        show_current_card (d, ED_TWEAK_ALL);
        return;
    }

    dialog_blank ();
}


static void
cb_m_reset_stats (GtkWidget *widget, Dialog *d)
{
    GList *item;


    /* Forget dates and counts for this card. */

    if (!ask_yes_or_no (GTK_WINDOW(d->window),
        _("<b>All dates and statistics for this card will be lost</b>\n\nContinue anyway?"),
        GTK_RESPONSE_NO)) {
        return;
    }

    item = file_get_current_item (d->ig->file);

    card_reset_statistics (CARD(item));
    ig_file_changed (d->ig);
    app_window_update_appbar (d->ig);

    app_window_refresh_card_row (d->ig, item);

    show_current_card (d, ED_TWEAK_INFO);
}


static void
cb_m_prev (GtkWidget *widget, Dialog *d)
{
    textview_check_changed (d);

    if (!prefs_get_sticky_flips (d->ig->prefs))
        gtk_toggle_button_set_active (d->b_flip, FALSE);

    file_decr_current_item (d->ig->file, TRUE);
    app_window_select_card (d->ig, file_get_current_item (d->ig->file));

    show_current_card (d, ED_TWEAK_ALL);
}


static void
cb_m_next (GtkWidget *widget, Dialog *d)
{
    textview_check_changed (d);

    if (!prefs_get_sticky_flips (d->ig->prefs))
        gtk_toggle_button_set_active (d->b_flip, FALSE);

    file_incr_current_item (d->ig->file, TRUE);
    app_window_select_card (d->ig, file_get_current_item (d->ig->file));

    show_current_card (d, ED_TWEAK_ALL);
}


static void
cb_m_flip (GtkCheckMenuItem *widget, Dialog *d)
{
    gboolean active;


    textview_check_changed (d);

    active = gtk_check_menu_item_get_active (widget);

    toggle_ui_flip (d, active);
    if (gtk_toggle_button_get_active (d->b_info) == FALSE)
        gtk_notebook_set_current_page (d->notebook, active ? BACK : FRONT);

    update_undo_sensitivity (d);
}


static void
cb_b_flip (GtkToggleButton *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_flip,
        gtk_toggle_button_get_active (widget));
    gtk_widget_grab_focus (GTK_WIDGET(widget));
}


static void
cb_m_sticky_flips (GtkCheckMenuItem *widget, Dialog *d)
{
    prefs_set_sticky_flips (d->ig->prefs,
        gtk_check_menu_item_get_active (widget));
}


static void
cb_m_switch_sides (GtkWidget *widget, Dialog *d)
{
    GList *item;


    textview_check_changed (d);

    card_switch_sides (file_get_current_card (d->ig->file));
    ig_file_changed (d->ig);
    show_current_card (d, ED_TWEAK_TEXTVIEW);

    /* Update this item in the main window's card list. */
    item = file_get_current_item (d->ig->file);
    app_window_refresh_card_row (d->ig, item);
}


static void
cb_m_flag (GtkCheckMenuItem *widget, Dialog *d)
{
    gboolean state;
    GList *item;
    Card *c;


    item = file_get_current_item (d->ig->file);

    c = CARD(item);

    state = gtk_check_menu_item_get_active (widget);

    if (card_get_flagged (c) != state) {
        card_set_flagged (c, state);
        ig_file_changed (d->ig);
        app_window_refresh_card_row (d->ig, item);
    }

    update_editor_title ();
}


static void
cb_m_close (GtkWidget *widget, Dialog *d)
{
    window_close (d);
}


static void
cb_m_cut (GtkWidget *w, Dialog *d)
{
    gint p;

    p = gtk_notebook_get_current_page (d->notebook);
    if (p != FRONT && p != BACK)
        return;

    gtk_text_buffer_cut_clipboard (d->textbuf[p],
        gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), TRUE);
}


static void
cb_m_copy (GtkWidget *w, Dialog *d)
{
    gint p;

    p = gtk_notebook_get_current_page (d->notebook);

    gtk_text_buffer_copy_clipboard (d->textbuf[p],
        gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}


static void
cb_m_paste (GtkWidget *w, Dialog *d)
{
    gint p;

    if (file_get_current_item (d->ig->file) == NULL)
        return;

    p = gtk_notebook_get_current_page (d->notebook);
    if (p != FRONT && p != BACK)
        return;

    gtk_text_buffer_paste_clipboard (d->textbuf[p],
        gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), NULL, TRUE);
    gtk_text_view_scroll_mark_onscreen (d->textview[p],
        gtk_text_buffer_get_insert (d->textbuf[p]));
}


static void
cb_m_delete (GtkWidget *w, Dialog *d)
{
    gint p;

    p = gtk_notebook_get_current_page (d->notebook);
    if (p != FRONT && p != BACK)
        return;

    gtk_text_buffer_delete_selection (d->textbuf[FRONT], TRUE, TRUE);
}


static void
cb_m_insert_media (GtkWidget *w, Dialog *d)
{
    gchar *fname;

    if ((fname = prompt_media_filename (d)) != NULL) {

        const gchar *type;
        gchar *s;
        gint p;

        p = gtk_notebook_get_current_page (d->notebook);
        if (p != FRONT && p != BACK)
            return;

        if (w == d->m_insert_image) {
            type = "image";
        }
        else if (w == d->m_insert_sound) {
            type = "sound";
        }
        else {
            g_assert_not_reached ();
        }

        /* XXX: What if fname contains a " char? */
        s = g_strdup_printf ("<embed type=\"%s\" src=\"%s\" alt=\"[%s]\"/>", type, fname, type);
        gtk_text_buffer_insert_interactive_at_cursor (d->textbuf[p],
            s, strlen (s), TRUE);

        g_free (fname);
        g_free (s);
    }
}


static void
cb_m_info (GtkCheckMenuItem *widget, Dialog *d)
{
    gboolean active;
    gint page;


    active = gtk_check_menu_item_get_active (widget);

    toggle_ui_info (d, active);

    if (active) {
        page = INFO;
    }
    else if (gtk_check_menu_item_get_active (d->m_flip)) {
        page = BACK;
    }
    else {
        page = FRONT;
    }
   
    gtk_notebook_set_current_page (d->notebook, page);

    update_undo_sensitivity (d);
    update_insert_sensitivity (d);
}


static void
cb_b_info (GtkToggleButton *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_info,
        gtk_toggle_button_get_active (widget));
    gtk_widget_grab_focus (GTK_WIDGET(widget));
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
cb_m_tag_bar (GtkCheckMenuItem *widget, Dialog *d)
{
    gboolean active;

    active =  gtk_check_menu_item_get_active (widget);
    if (active) {
        gtk_widget_show (d->hbox_tag_bar);
        gtk_widget_grab_focus (d->entry_tag_bar);
    }
    else {
        gtk_widget_hide (d->hbox_tag_bar);
    }

    prefs_set_editor_tag_bar_visible (d->ig->prefs, active);
}


static void
cb_b_close_tag_bar (GtkWidget *widget, Dialog *d)
{
    gtk_check_menu_item_set_active (d->m_tag_bar, FALSE);
    gtk_widget_hide (d->hbox_tag_bar);

    prefs_set_editor_tag_bar_visible (d->ig->prefs, FALSE);
}


static void
cb_tag_bar_activate (GtkWidget *widget, Dialog *d)
{
    GList *item;
    const gchar *s;
    gboolean changed;
    Card *c;


    item = file_get_current_item (d->ig->file);

    c = CARD(item);

    s = gtk_entry_get_text (GTK_ENTRY(d->entry_tag_bar));
    if (strcmp (d->s_tag_bar, s) != 0)
        changed = file_card_set_tags_from_string (d->ig->file, c, s);

    if (changed) {
        ig_file_changed (d->ig);
        /* app_window_refresh_card_row (d->ig, item); */
        dialog_tagger_tweak ();
    }
}


static void
cb_b_clear_tag_bar (GtkWidget *widget, Dialog *d)
{
    gtk_entry_set_text (GTK_ENTRY(d->entry_tag_bar), "");
    cb_tag_bar_activate (widget, d);
}


static void
cb_window_delete (GtkWidget *widget, GdkEvent *event, Dialog *d)
{
    window_close (d);
}


static void
cb_textbuf_modified (GtkTextBuffer *buf, Dialog *d)
{
    ig_file_changed (d->ig);
}


void
dialog_editor_check_changed (void)
{
    if (dialog != NULL)
        textview_check_changed (dialog);
}


void
dialog_editor_kill (void)
{
    if (dialog != NULL)
        window_close (dialog);
}


void
dialog_editor_tweak (EdTweak tweak)
{
    if (dialog == NULL)
        return;

    if (!file_get_current_item (dialog->ig->file)) {
        dialog_blank ();
        return;
    }

    show_current_card (dialog, tweak);
}


static void
update_textview_properties (Dialog *d)
{
    textview_set_properties (d->textview[FRONT], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_EDITOR);
    textview_set_properties (d->textview[BACK], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_EDITOR);
    textview_set_properties (d->textview[INFO], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_INFO);
}


void
dialog_editor_preferences_changed (void)
{
    if (dialog == NULL)
        return;

    if (!file_get_current_item (dialog->ig->file)) {
        dialog_blank ();
        return;
    }

    update_textview_properties (dialog);
}


void
dialog_editor_card_style_changed (void)
{
    CardStyle style;

    if (dialog == NULL)
        return;

    style = file_get_card_style (dialog->ig->file);

    gtk_text_view_set_justification (GTK_TEXT_VIEW(dialog->textview[FRONT]),
        card_style_get_justification (style));
    gtk_text_view_set_justification (GTK_TEXT_VIEW(dialog->textview[BACK]),
        card_style_get_justification (style));
}


gboolean
cb_key_press (GtkWidget *w, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_Escape) {
        g_print ("beep\n");
    }
    return FALSE;
}


void
dialog_editor (Ignuit *ig)
{
    Dialog     *d;
    GtkWidget  *m_close, *b_close_tag_bar;
    GtkWidget  *m_sticky_flips;
    gchar      *glade_file;
    GladeXML   *glade_xml;


    if (dialog != NULL) {
        gtk_window_present (GTK_WINDOW(dialog->window));
        show_current_card (dialog, ED_TWEAK_ALL);
        return;
    }

    glade_file = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_EDITOR, TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_EDITOR);
        return;
    }

    dialog = d = g_new0 (Dialog, 1);
    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;


    d->window = glade_xml_get_widget (glade_xml, "dialog");

    g_signal_connect (G_OBJECT(d->window), "key-press-event",
        G_CALLBACK(cb_window_key_press_event), d);

    gtk_window_set_transient_for (GTK_WINDOW(d->window),
        GTK_WINDOW(ig->app));
    gtk_window_set_modal (GTK_WINDOW(d->window), FALSE);
    gtk_window_set_default_size (GTK_WINDOW(d->window),
        prefs_get_quiz_width (d->ig->prefs),
        prefs_get_quiz_height (d->ig->prefs));

    g_signal_connect (G_OBJECT(d->window), "delete-event",
        G_CALLBACK(cb_window_delete), d);

    d->statusbar = (GtkStatusbar*)glade_xml_get_widget
        (glade_xml, "statusbar");
    d->sbcid = gtk_statusbar_get_context_id (d->statusbar, "editor");


    d->notebook = GTK_NOTEBOOK(glade_xml_get_widget (glade_xml, "notebook"));


    /* Front textview */

    d->textview[FRONT] = GTK_TEXT_VIEW(glade_xml_get_widget
        (glade_xml, "tv_front"));
    d->textbuf[FRONT] = gtk_text_view_get_buffer (d->textview[FRONT]);
    textview_set_properties (d->textview[FRONT], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_EDITOR);

    g_signal_connect (G_OBJECT(d->textview[FRONT]), "key-press-event",
        G_CALLBACK(cb_textview_key_press_event), d);
    g_signal_connect_after (G_OBJECT(d->textbuf[FRONT]), "insert-text",
        G_CALLBACK(cb_textbuf_insert_text), d);
    g_signal_connect (G_OBJECT(d->textbuf[FRONT]), "delete-range",
        G_CALLBACK(cb_textbuf_delete_range), d);
    g_signal_connect_after (G_OBJECT(d->textbuf[FRONT]), "end-user-action",
        G_CALLBACK(cb_textbuf_end_user_action), d);
    g_signal_connect_after (G_OBJECT(d->textbuf[FRONT]), "modified_changed",
        G_CALLBACK(cb_textbuf_modified), d);


    /* Back textview */

    d->textview[BACK] = GTK_TEXT_VIEW(glade_xml_get_widget
        (glade_xml, "tv_back"));
    d->textbuf[BACK] = gtk_text_view_get_buffer (d->textview[BACK]);
    textview_set_properties (d->textview[BACK], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_EDITOR);

    g_signal_connect (G_OBJECT(d->textview[BACK]), "key-press-event",
        G_CALLBACK(cb_textview_key_press_event), d);
    g_signal_connect_after (G_OBJECT(d->textbuf[BACK]), "insert-text",
        G_CALLBACK(cb_textbuf_insert_text), d);
    g_signal_connect (G_OBJECT(d->textbuf[BACK]), "delete-range",
        G_CALLBACK(cb_textbuf_delete_range), d);
    g_signal_connect_after (G_OBJECT(d->textbuf[BACK]), "end-user-action",
        G_CALLBACK(cb_textbuf_end_user_action), d);
    g_signal_connect_after (G_OBJECT(d->textbuf[BACK]), "modified_changed",
        G_CALLBACK(cb_textbuf_modified), d);


    /* Details textview */

    d->textview[INFO] = GTK_TEXT_VIEW(glade_xml_get_widget
        (glade_xml, "tv_info"));
    d->textbuf[INFO] = gtk_text_view_get_buffer (d->textview[INFO]);
    textbuf_create_tags (d->textbuf[INFO]);
    textview_set_properties (d->textview[INFO], d->ig->prefs,
        file_get_card_style (d->ig->file), TV_INFO);



    /* Card menu */

    d->m_add = glade_xml_get_widget (glade_xml, "m_add");
    g_signal_connect (G_OBJECT(d->m_add), "activate",
        G_CALLBACK(cb_m_add), d);

    d->m_remove = glade_xml_get_widget (glade_xml, "m_remove");
    g_signal_connect (G_OBJECT(d->m_remove), "activate",
        G_CALLBACK(cb_m_remove), d);

    d->m_prev = glade_xml_get_widget (glade_xml, "m_prev");
    g_signal_connect (G_OBJECT(d->m_prev), "activate",
        G_CALLBACK(cb_m_prev), d);

    d->m_next = glade_xml_get_widget (glade_xml, "m_next");
    g_signal_connect (G_OBJECT(d->m_next), "activate",
        G_CALLBACK(cb_m_next), d);

    d->m_flip = (GtkCheckMenuItem*)glade_xml_get_widget (glade_xml, "m_flip");
    g_signal_connect (G_OBJECT(d->m_flip), "toggled",
        G_CALLBACK(cb_m_flip), d);

    d->m_info = (GtkCheckMenuItem*)glade_xml_get_widget (glade_xml, "m_info");
    g_signal_connect (G_OBJECT(d->m_info), "toggled",
        G_CALLBACK(cb_m_info), d);

    m_sticky_flips = glade_xml_get_widget (glade_xml, "m_sticky_flips");
    g_signal_connect (G_OBJECT(m_sticky_flips), "toggled",
        G_CALLBACK(cb_m_sticky_flips), d);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(m_sticky_flips),
        prefs_get_sticky_flips (d->ig->prefs));

    d->m_tag_bar = (GtkCheckMenuItem*)glade_xml_get_widget (glade_xml,
        "m_tag_bar");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(d->m_tag_bar),
        prefs_get_editor_tag_bar_visible (d->ig->prefs));

    g_signal_connect (G_OBJECT(d->m_tag_bar), "toggled",
        G_CALLBACK(cb_m_tag_bar), d);

    m_close = glade_xml_get_widget (glade_xml, "m_close");
    g_signal_connect (G_OBJECT(m_close), "activate",
        G_CALLBACK(cb_m_close), d);


    /* Edit menu */

    d->m_undo = glade_xml_get_widget (glade_xml, "m_undo");
    g_signal_connect (G_OBJECT(d->m_undo), "activate",
        G_CALLBACK(cb_m_undo), d);

    d->m_redo = glade_xml_get_widget (glade_xml, "m_redo");
    g_signal_connect (G_OBJECT(d->m_redo), "activate",
        G_CALLBACK(cb_m_redo), d);

    d->m_cut = glade_xml_get_widget (glade_xml, "m_cut");
    g_signal_connect (G_OBJECT(d->m_cut), "activate",
        G_CALLBACK(cb_m_cut), d);

    d->m_copy = glade_xml_get_widget (glade_xml, "m_copy");
    g_signal_connect (G_OBJECT(d->m_copy), "activate",
        G_CALLBACK(cb_m_copy), d);

    d->m_delete = glade_xml_get_widget (glade_xml, "m_delete");
    g_signal_connect (G_OBJECT(d->m_delete), "activate",
        G_CALLBACK(cb_m_delete), d);

    d->m_paste = glade_xml_get_widget (glade_xml, "m_paste");
    g_signal_connect (G_OBJECT(d->m_paste), "activate",
        G_CALLBACK(cb_m_paste), d);

    d->m_insert_image = glade_xml_get_widget (glade_xml, "m_insert_image");
    g_signal_connect (G_OBJECT(d->m_insert_image), "activate",
        G_CALLBACK(cb_m_insert_media), d);

    d->m_insert_sound = glade_xml_get_widget (glade_xml, "m_insert_sound");
    g_signal_connect (G_OBJECT(d->m_insert_sound), "activate",
        G_CALLBACK(cb_m_insert_media), d);

    d->m_flag = (GtkCheckMenuItem*)glade_xml_get_widget (glade_xml, "m_flag");
    g_signal_connect (G_OBJECT(d->m_flag), "toggled",
        G_CALLBACK(cb_m_flag), d);

    d->m_switch_sides = glade_xml_get_widget (glade_xml, "m_switch_sides");
    g_signal_connect (G_OBJECT(d->m_switch_sides), "activate",
        G_CALLBACK(cb_m_switch_sides), d);

    d->m_reset_stats = glade_xml_get_widget (glade_xml, "m_reset_stats");
    g_signal_connect (G_OBJECT(d->m_reset_stats), "activate",
        G_CALLBACK(cb_m_reset_stats), d);


    /* Tag bar */

    d->hbox_tag_bar = glade_xml_get_widget (glade_xml, "hbox_tag_bar");
    d->entry_tag_bar = glade_xml_get_widget (glade_xml, "entry_tag_bar");
    d->b_clear_tag_bar = glade_xml_get_widget (glade_xml, "b_clear_tag_bar");
    b_close_tag_bar = glade_xml_get_widget (glade_xml, "b_close_tag_bar");

    g_signal_connect (G_OBJECT(b_close_tag_bar), "clicked",
        G_CALLBACK(cb_b_close_tag_bar), d);
    g_signal_connect (G_OBJECT(d->b_clear_tag_bar), "clicked",
        G_CALLBACK(cb_b_clear_tag_bar), d);
    g_signal_connect (G_OBJECT(d->entry_tag_bar), "activate",
        G_CALLBACK(cb_tag_bar_activate), d);

    if (!prefs_get_editor_tag_bar_visible (d->ig->prefs))
        gtk_widget_hide (d->hbox_tag_bar);


    /* Navigation and scoring buttons */

    d->b_info = (GtkToggleButton*)glade_xml_get_widget (glade_xml, "b_info");
    g_signal_connect (G_OBJECT(d->b_info), "toggled",
        G_CALLBACK(cb_b_info), d);

    d->b_add = glade_xml_get_widget (glade_xml, "b_add");
    g_signal_connect (G_OBJECT(d->b_add), "clicked",
        G_CALLBACK(cb_m_add), d);

    d->b_remove = glade_xml_get_widget (glade_xml, "b_remove");
    g_signal_connect (G_OBJECT(d->b_remove), "clicked",
        G_CALLBACK(cb_m_remove), d);

    d->b_prev = glade_xml_get_widget (glade_xml, "b_prev");
    g_signal_connect (G_OBJECT(d->b_prev), "clicked",
        G_CALLBACK(cb_m_prev), d);

    d->b_next = glade_xml_get_widget (glade_xml, "b_next");
    g_signal_connect (G_OBJECT(d->b_next), "clicked",
        G_CALLBACK(cb_m_next), d);

    d->b_flip = (GtkToggleButton*)glade_xml_get_widget (glade_xml, "b_flip");
    g_signal_connect (G_OBJECT(d->b_flip), "toggled",
        G_CALLBACK(cb_b_flip), d);

    show_current_card (d, ED_TWEAK_ALL);

    gtk_widget_show (d->window);

    g_object_unref (G_OBJECT(glade_xml));
}

