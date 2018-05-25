/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2008, 2009, 2012, 2015, 2016, 2017 Timothy Richard Musson
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
#include "app-window.h"
#include "dialog-properties.h"
#include "dialog-category-properties.h"
#include "dialog-preferences.h"
#include "dialog-editor.h"
#include "dialog-tagger.h"
#include "dialog-quiz.h"
#include "dialog-find.h"
#include "dialog-about.h"
#include "fileio.h"
#include "textview.h"


typedef struct {

    Ignuit           *ig;

    GtkWidget        *window;

    GtkWidget        *scrollw_categories;
    GtkTreeSelection *sel_categories;

    GtkWidget        *scrollw_cards;
    GtkTreeSelection *sel_cards;

    GtkWidget        *t_find;
    GtkWidget        *t_category_previous;
    GtkWidget        *t_category_next;
    GtkWidget        *t_start_quiz;

    GtkWidget        *m_reset_stats;
    GtkWidget        *m_edit_card;
    GtkWidget        *m_add_card;
    GtkWidget        *m_remove_card;
    GtkWidget        *m_cut_card;
    GtkWidget        *m_copy_card;
    GtkWidget        *m_paste_card;
    GtkWidget        *m_select_all;
    GtkWidget        *m_find;
    GtkWidget        *m_find_all;
    GtkWidget        *m_find_flagged;
    GtkWidget        *m_view_trash;

    GtkWidget        *m_remove_category;
    GtkWidget        *m_category_previous;
    GtkWidget        *m_category_next;
    GtkWidget        *m_category_properties;

    GtkWidget        *m_edit_tags;
    GtkWidget        *m_flag;
    GtkWidget        *m_switch_sides;

    GtkWidget        *m_start_quiz;
    GtkWidget        *m_start_drill;

    GtkWidget        *r_quiz_category_selection;
    GSList           *quiz_category_group;

    GtkWidget        *r_quiz_card_selection;
    GSList           *quiz_card_group;

    GtkWidget        *r_quiz_face_selection;
    GSList           *quiz_face_group;

    GtkWidget        *m_quiz_in_order;

    GtkWidget        *b_remove_category;
    GtkWidget        *b_edit_card;
    GtkWidget        *b_add_card;
    GtkWidget        *b_remove_card;

    GtkWidget        *popup_menu_category;
    GtkWidget        *m_category_popup_rename;
    GtkWidget        *m_category_popup_remove;
    GtkWidget        *m_category_popup_toggle_fixed_order;
    GtkWidget        *m_category_popup_properties;

    GtkWidget        *popup_menu_card;
    GtkWidget        *m_card_popup_add;
    GtkWidget        *m_card_popup_remove;
    GtkWidget        *m_card_popup_edit;
    GtkWidget        *m_card_popup_cut;
    GtkWidget        *m_card_popup_copy;
    GtkWidget        *m_card_popup_paste;
    GtkWidget        *m_card_popup_select_all;
    GtkWidget        *m_card_popup_edit_tags;
    GtkWidget        *m_card_popup_flag;
    GtkWidget        *m_card_popup_switch_sides;
    GtkWidget        *m_card_popup_reset_stats;

    GtkWidget        *popup_menu_card_header;
    GtkWidget        *m_card_header_toggle[CARDS_N_COLUMNS];

    GtkWidget        *hpaned;

    GtkTreeViewColumn *treev_cat_title_column;
    GtkTreeViewColumn *treev_card_col[CARDS_N_COLUMNS];

    GList            *drag_list;

    guint            poll_timeout_id;

    GList            *last_selected_category;

} AppWin;


static GtkWidget *toolbar, *category_pane, *appbar;
static GtkWidget *category_buttonbox, *card_buttonbox;


/* Combobox IDs for import/export filter selection: */
enum {
    FILTER_CSV = 0,
    FILTER_TSV,
    FILTER_NATIVE,
    FILTER_XSLT
};


/* Drag'n'drop */
static GtkTargetEntry target_table[] = {
    { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, 0 },
    { "text/plain", GTK_TARGET_SAME_APP, 1 }
};


typedef void (*FrobulationFunc)(AppWin *d, GtkTreeModel *model,
    GtkTreeIter *iter, gboolean *changed);

static void cb_category_row_inserted (GtkTreeModel *tree_model,
    GtkTreePath *path, GtkTreeIter *iter, AppWin *d);

#if 0
static gboolean current_category_is_first (AppWin *d);
static gboolean current_category_is_last (AppWin *d);
#endif


static gchar*
truncated_card_text (const gchar *text)
{
    gchar *nl;

    /* Return a copy of text, starting at the first non-whitespace
     * character, then ending at the first '\n'. */

    while (*text != '\0' && g_ascii_isspace (*text))
        text++;

    if ((nl = strchr (text, '\n')) == NULL)
        return g_strdup (text);

    return g_strndup (text, nl - text);
}


static void
cell_func_card_expired (GtkTreeViewColumn *col,
    GtkCellRenderer *renderer,
    GtkTreeModel    *model,
    GtkTreeIter     *iter,
    gpointer        data)
{
    static gboolean expired;
    static AppWin *d;

    /* Indicate expired cards. */

    gtk_tree_model_get (model, iter, COLUMN_CARD_EXPIRED, &expired, -1);
    if (expired) {

        d = (AppWin*)data;

        g_object_set (renderer, "foreground", d->ig->color_expiry, NULL);
        g_object_set (renderer, "text", UNICHAR_EXPIRED, NULL);

    }
    else {

        g_object_set (renderer, "text", "", NULL);

    }
}


static void
cell_func_card_flagged (GtkTreeViewColumn *col,
    GtkCellRenderer *renderer,
    GtkTreeModel    *model,
    GtkTreeIter     *iter,
    gpointer        data)
{
    static gboolean flagged;

    /* Indicate flagged cards. */

    gtk_tree_model_get (model, iter, COLUMN_CARD_FLAGGED, &flagged, -1);
    g_object_set (renderer, "text", flagged ? UNICHAR_FLAGGED : "", NULL);
}


static void
cell_func_category_card_count (GtkTreeViewColumn *col,
    GtkCellRenderer *renderer,
    GtkTreeModel    *model,
    GtkTreeIter     *iter,
    gpointer        data)
{
    static AppWin *d;
    GList *item;

    d = (AppWin*)data;

    gtk_tree_model_get (model, iter, COLUMN_CATEGORY_DATA, &item, -1);
    if (category_get_n_expired (CATEGORY(item)) > 0) {
        g_object_set (renderer, "foreground", d->ig->color_expiry, NULL);
    }
    else {
        /* Use the system theme's default text colour. */
        g_object_set (renderer, "foreground", d->ig->color_plain, NULL);
    }
}


static void
cell_func_card_group (GtkTreeViewColumn *col,
    GtkCellRenderer *renderer,
    GtkTreeModel    *model,
    GtkTreeIter     *iter,
    gpointer        data)
{
    static gchar s[2];
    static Group group;


    gtk_tree_model_get (model, iter, COLUMN_CARD_GROUP, &group, -1);

    s[0] = (group == GROUP_NEW) ? 'N' : '0' + group;
    s[1] = '\0';

    g_object_set (renderer, "text", s, NULL);
}


static void
cell_func_card_expiry_time (GtkTreeViewColumn *col,
    GtkCellRenderer *renderer,
    GtkTreeModel    *model,
    GtkTreeIter     *iter,
    gpointer        data)
{
    static gchar s[6];
    static gint hour;
    static Group group;

    gtk_tree_model_get (model, iter, COLUMN_CARD_GROUP, &group, -1);
    if (group == GROUP_NEW) {
        s[0] = '-';
        s[1] = '\0';
    }
    else {
        gtk_tree_model_get (model, iter, COLUMN_CARD_EXPIRY_TIME, &hour, -1);
        g_snprintf (s, 6, "%02d:00", hour);
    }

    g_object_set (renderer, "text", s, NULL);
}


#define COL_DATA 0

static void
add_selected_card_to_list (GtkTreeModel *model,
    GtkTreePath *path, GtkTreeIter *iter, GList **list)
{
    GList *item;

    gtk_tree_model_get (model, iter, COL_DATA, &item, -1);
    *list = g_list_append (*list, item->data);
}


GList*
treev_get_selected_items (GtkTreeView *treev)
{
    GtkTreeSelection *sel;
    GList *list = NULL;


    /* Return a list items selected in this tree view. */

    sel = gtk_tree_view_get_selection (treev);
    gtk_tree_selection_selected_foreach (sel,
        (GtkTreeSelectionForeachFunc)add_selected_card_to_list, &list);

    return list;
}


void
app_window_update_appbar (Ignuit *ig)
{
    Category *cat;
    GString  *gstr;
    guint    nf, nc;
    gchar    *s, *s2 = NULL;


    nf = file_get_n_cards (ig->file);

    cat = file_get_current_category (ig->file);

    if (cat == NULL) {
        s = g_strdup_printf (_("File contains %d cards"), nf);
    }
    else if (file_category_is_trash (ig->file, cat)) {
        s = g_strdup_printf (_("Trash contains %d cards"),
            category_get_n_cards (cat));
    }
    else if (file_category_is_search (ig->file, cat)) {
        s = g_strdup_printf (_("Found %d cards"),
            category_get_n_cards (cat));
    }
    else {
        nc = category_get_n_cards (cat);
        s = g_strdup_printf (_("Category contains %d of %d cards"), nc, nf);
    }

    if (cat != NULL) {
        s2 = g_strdup_printf (_(" (%d expired, %d untested, %d selected)"),
            category_get_n_expired (cat),
            category_get_n_untested (cat),
            ig->n_cards_selected);
    }

    gstr = g_string_new (s);
    g_free (s);

    if (s2 != NULL) {
        g_string_append (gstr, s2);
        g_free (s2);
    }

    gnome_appbar_set_status (ig->appbar, gstr->str);

    g_string_free (gstr, TRUE);
}


void
app_window_update_title (Ignuit *ig)
{
    gboolean changed;
    const gchar *fname;
    gchar *s, *title;

    if ((fname = file_get_title (ig->file)) != NULL) {
        s = g_strdup (fname);
    }
    else if ((fname = file_get_filename (ig->file)) != NULL) {
        s = g_filename_display_basename (fname);
    }
    else {
        s = g_strdup ("i GNU it");
    }

    changed = file_get_changed (ig->file);

    if (changed) {
        title = g_strdup_printf ("*%s", s);
    }
    else {
        title = g_strdup (s);
    }

    gtk_window_set_title (GTK_WINDOW(ig->app), title);

    gtk_widget_set_sensitive (ig->t_save, changed);
    gtk_widget_set_sensitive (ig->m_save, changed);

    g_free (title);
    g_free (s);
}


static void
app_window_update_sensitivity (AppWin *d)
{
    Category *cat;
    gboolean file_has_cards;
    gboolean category_has_cards;
    gboolean can_paste;
    gboolean can_add;
    gboolean is_special_category;
    gboolean have_previous_category;
    gboolean have_next_category;


    /* XXX: Here we sync file->category_order with the category order displayed
     * in the category pane. Better if it was always kept in sync by the
     * functions that manage the category pane. */
    file_set_category_order (d->ig->file, app_window_get_category_list (d->ig));

    cat = file_get_current_category (d->ig->file);

    is_special_category = file_category_is_search (d->ig->file, cat)
        || file_category_is_trash (d->ig->file, cat)
        || ig_category_is_clipboard (d->ig, cat);

    can_add = !(cat == NULL || is_special_category);

    file_has_cards = (file_get_n_cards (d->ig->file) > 0);
    category_has_cards = (cat && category_get_n_cards (cat) > 0);

    gtk_widget_set_sensitive (d->m_find_all, file_has_cards);
    gtk_widget_set_sensitive (d->m_find_flagged, file_has_cards);
    gtk_widget_set_sensitive (d->t_start_quiz, file_has_cards);
    gtk_widget_set_sensitive (d->m_start_quiz, file_has_cards);
    gtk_widget_set_sensitive (d->m_start_drill, file_has_cards);
    gtk_widget_set_sensitive (d->b_add_card, can_add);
    gtk_widget_set_sensitive (d->m_add_card, can_add);
    gtk_widget_set_sensitive (d->m_card_popup_add, can_add);
    gtk_widget_set_sensitive (d->m_view_trash,
        cat != NULL && file_get_trash (d->ig->file));
    gtk_widget_set_sensitive (d->b_remove_category, can_add);
    gtk_widget_set_sensitive (d->m_remove_category, can_add);
    gtk_widget_set_sensitive (d->m_category_popup_rename, can_add);
    gtk_widget_set_sensitive (d->m_category_popup_remove, can_add);
    gtk_widget_set_sensitive (d->m_category_popup_toggle_fixed_order, can_add);
    gtk_widget_set_sensitive (d->m_category_popup_properties, can_add);
    gtk_widget_set_sensitive (d->m_edit_tags, category_has_cards);
    gtk_widget_set_sensitive (d->m_category_properties, can_add);

    gtk_widget_set_sensitive (d->m_flag, category_has_cards);
    gtk_widget_set_sensitive (d->m_switch_sides, category_has_cards);
    gtk_widget_set_sensitive (d->m_card_popup_edit_tags,
        category_has_cards);
    gtk_widget_set_sensitive (d->m_card_popup_flag,
        category_has_cards);
    gtk_widget_set_sensitive (d->m_card_popup_switch_sides,
        category_has_cards);
    gtk_widget_set_sensitive (d->m_reset_stats, category_has_cards);
    gtk_widget_set_sensitive (d->m_card_popup_reset_stats,
        category_has_cards);
    gtk_widget_set_sensitive (d->m_select_all, category_has_cards);
    gtk_widget_set_sensitive (d->m_card_popup_select_all,
        category_has_cards);
    gtk_widget_set_sensitive (d->m_find, file_has_cards);
    gtk_widget_set_sensitive (d->t_find, file_has_cards);

    have_previous_category = !file_current_category_is_first (d->ig->file);
    have_next_category = !file_current_category_is_last (d->ig->file);

    gtk_widget_set_sensitive (d->m_category_previous, have_previous_category
        || (is_special_category && file_has_cards));
    gtk_widget_set_sensitive (d->m_category_next, have_next_category
        && !is_special_category);
    gtk_widget_set_sensitive (d->t_category_previous, have_previous_category
        || (is_special_category && file_has_cards));
    gtk_widget_set_sensitive (d->t_category_next, have_next_category
        && !is_special_category);

    can_paste = can_add && (ig_get_clipboard (d->ig) != NULL);

    gtk_widget_set_sensitive (d->m_paste_card, can_paste);
    gtk_widget_set_sensitive (d->m_card_popup_paste, can_paste);

    app_window_update_appbar (d->ig);
}


static void
app_window_refresh_category_pane (Ignuit *ig)
{
    GList *cur;
    GtkTreeIter iter;
    Category *cat;
    GtkTreeModel *model;


    /* Clear and re-fill the category treeview. */

    model = gtk_tree_view_get_model (ig->treev_cat);
    gtk_list_store_clear (GTK_LIST_STORE(model));

    for (cur = file_get_categories(ig->file); cur != NULL; cur = cur->next) {

        cat = CATEGORY(cur);

        gtk_list_store_append (GTK_LIST_STORE(model), &iter);
        gtk_list_store_set (GTK_LIST_STORE(model), &iter,
            COLUMN_CATEGORY_DATA, cur,
            COLUMN_CATEGORY_TITLE, category_get_title (cat),
            COLUMN_CATEGORY_CARD_COUNT, category_get_n_cards (cat),
            -1);
    }
}


void
app_window_refresh_card_pane (Ignuit *ig, GList *cards)
{
    static gchar       tst[11], exp[11];
    gchar              *front, *back;
    static GtkTreeIter iter;
    GList              *cur;
    GtkTreeModel       *model;
    Card               *c;


    /* Clear and re-fill the card treeview. */

    /* For speed, temporarily detach the model from the view. */
    model = gtk_tree_view_get_model (ig->treev_card);
    g_object_ref (model);
    gtk_tree_view_set_model (ig->treev_card, NULL);

    gtk_list_store_clear (GTK_LIST_STORE(model));

    for (cur = cards; cur != NULL; cur = cur->next) {

        c = CARD(cur);

        date_str (tst, card_get_gdate_tested (c));
        date_str (exp, card_get_gdate_expiry (c));

        front = truncated_card_text (card_get_front_without_markup (c));
        back = truncated_card_text (card_get_back_without_markup (c));

        gtk_list_store_append (GTK_LIST_STORE(model), &iter);
        gtk_list_store_set (GTK_LIST_STORE(model), &iter,
            COLUMN_CARD_DATA, cur,
            COLUMN_CARD_FRONT, front,
            COLUMN_CARD_BACK, back,
            COLUMN_CARD_CATEGORY, category_get_title (card_get_category (c)),
            COLUMN_CARD_GROUP, card_get_group (c),
            COLUMN_CARD_TESTED, tst,
            COLUMN_CARD_EXPIRED, card_get_expired (c),
            COLUMN_CARD_EXPIRY_DATE, exp,
            COLUMN_CARD_EXPIRY_TIME, card_get_time_expiry (c),
            COLUMN_CARD_FLAGGED, card_get_flagged (c),
            -1);

        g_free (front);
        g_free (back);
    }

    /* Re-attach model to view. */
    gtk_tree_view_set_model (ig->treev_card, model);
    g_object_unref (model);
}


GList*
app_window_get_category_list (Ignuit *ig)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;
    GList *item, *list = NULL;

    /* Return a list of categories in the order displayed
     * in the category pane. */

    model = gtk_tree_view_get_model (ig->treev_cat);
    valid = gtk_tree_model_get_iter_first (model, &iter);

    while (valid) {

        gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);
        g_assert (item != NULL);

        list = g_list_append (list, item->data);
        valid = gtk_tree_model_iter_next (model, &iter);

    }

    return list;
}


GList*
app_window_find_treev_iter_with_data (GtkTreeView *treev,
    GtkTreeIter *iter, gpointer data)
{
    GtkTreeModel *model;
    gboolean valid;
    GList *item;


    /* Find the search data in the treeview, and set iter to it.
     * Return the found item, or NULL. */

    model = gtk_tree_view_get_model (treev);
    valid = gtk_tree_model_get_iter_first (model, iter);

    while (valid) {

        gtk_tree_model_get (model, iter, COLUMN_DATA, &item, -1);
        if (item->data == data)
            return item;

        valid = gtk_tree_model_iter_next (model, iter);

    }
    return NULL;
}


void
app_window_set_category_iter (Ignuit *ig, GtkTreeIter *iter, GList *item)
{
    GtkTreeModel *model;
    Category *cat;

    cat = CATEGORY(item);

    model = gtk_tree_view_get_model (ig->treev_cat);
    gtk_list_store_set (GTK_LIST_STORE(model), iter,
        COLUMN_CATEGORY_DATA, item,
        COLUMN_CATEGORY_TITLE, category_get_title (cat),
        COLUMN_CATEGORY_CARD_COUNT, category_get_n_cards (cat),
        -1);
}


void
app_window_refresh_category_row (Ignuit *ig, Category *cat)
{
    GtkTreeIter iter;
    GList *item;

    /* Make sure the row for this item is displaying current information. */

    item = app_window_find_treev_iter_with_data (ig->treev_cat, &iter, cat);
    app_window_set_category_iter (ig, &iter, item);
}


void
app_window_set_card_iter (Ignuit *ig, GtkTreeIter *iter, GList *item)
{
    gchar        tst[11], exp[11];
    gchar        *front, *back;
    GtkTreeModel *model;
    Card         *c;


    /* Link this iter to the given item. */

    c = CARD(item);

    date_str (tst, card_get_gdate_tested (c));
    date_str (exp, card_get_gdate_expiry (c));

    front = truncated_card_text (card_get_front_without_markup (c));
    back = truncated_card_text (card_get_back_without_markup (c));

    model = gtk_tree_view_get_model (ig->treev_card);
    gtk_list_store_set (GTK_LIST_STORE(model), iter,
        COLUMN_CARD_DATA, item,
        COLUMN_CARD_FRONT, front,
        COLUMN_CARD_BACK, back,
        COLUMN_CARD_CATEGORY, category_get_title (card_get_category (c)),
        COLUMN_CARD_GROUP, card_get_group (c),
        COLUMN_CARD_TESTED, tst,
        COLUMN_CARD_EXPIRED, card_get_expired (c),
        COLUMN_CARD_EXPIRY_DATE, exp,
        COLUMN_CARD_EXPIRY_TIME, card_get_time_expiry (c),
        COLUMN_CARD_FLAGGED, card_get_flagged (c),
        -1);

    g_free (front);
    g_free (back);
}


static void
app_window_redraw_card_list (Ignuit *ig)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GList *item;

    model = gtk_tree_view_get_model (ig->treev_card);

    if (!gtk_tree_model_get_iter_first (model, &iter))
        return;

    gtk_tree_model_get (model, &iter, COLUMN_CARD_DATA, &item, -1);
    app_window_set_card_iter (ig, &iter, item);

    while (gtk_tree_model_iter_next (model, &iter)) {
        gtk_tree_model_get (model, &iter, COLUMN_CARD_DATA, &item, -1);
        app_window_set_card_iter (ig, &iter, item);
    }
}


static void
app_window_redraw_category_list (Ignuit *ig)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GList *item;

    model = gtk_tree_view_get_model (ig->treev_cat);

    if (!gtk_tree_model_get_iter_first (model, &iter))
        return;

    gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);
    app_window_set_category_iter (ig, &iter, item);

    while (gtk_tree_model_iter_next (model, &iter)) {
        gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);
        app_window_set_category_iter (ig, &iter, item);
    }
}


void
app_window_refresh_card_row (Ignuit *ig, GList *item)
{
    GtkTreeIter iter;

    /* Make sure the row for this item is displaying current information. */

    app_window_find_treev_iter_with_data (ig->treev_card, &iter, CARD(item));
    app_window_set_card_iter (ig, &iter, item);
}


void
app_window_add_card_iter (Ignuit *ig, GtkTreeIter *iter, GList *item)
{
    GtkTreeModel *model;
    GtkTreeIter sibling;


    /* Add new card to the treeview, and select it. */

    model = gtk_tree_view_get_model (ig->treev_card);

    if (gtk_tree_model_get_iter_first (model, &sibling))
        gtk_list_store_insert_before (GTK_LIST_STORE(model), iter, &sibling);
    else
        gtk_list_store_append (GTK_LIST_STORE(model), iter);

    app_window_set_card_iter (ig, iter, item);
    app_window_select_card (ig, item);
}


void
app_window_select_treev_iter (GtkTreeView *treev, GtkTreeIter *iter)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreePath *path;


    /* Select and scroll to a TreeView iter. */

    sel = gtk_tree_view_get_selection (treev);
    gtk_tree_selection_unselect_all (sel);

    model = gtk_tree_view_get_model (treev);
    path = gtk_tree_model_get_path (model, iter);

    gtk_tree_selection_select_iter (sel, iter);
    gtk_tree_view_scroll_to_cell (treev, path, NULL, TRUE, 0.5, 0.0);
}


gboolean
app_window_select_card (Ignuit *ig, GList *item)
{
    GtkTreeIter iter;
    gboolean ok;


    /* Select and scroll to an item in the card TreeView. */

    if (item == NULL) {

        GtkTreeSelection *sel;

        sel = gtk_tree_view_get_selection (ig->treev_card);
        gtk_tree_selection_unselect_all (sel);
        file_set_current_item (ig->file, NULL);

        return TRUE;
    }

    ok = (app_window_find_treev_iter_with_data
        (ig->treev_card, &iter, CARD(item)) != NULL);

    if (ok)
        app_window_select_treev_iter (ig->treev_card, &iter);

    return ok;
}


gboolean
app_window_select_category (Ignuit *ig, GList *item)
{
    GtkTreeIter iter;
    gboolean ok;


    /* Select and scroll to an item in the category TreeView. */

    if (item == NULL) {

        GtkTreeSelection *sel;

        sel = gtk_tree_view_get_selection (ig->treev_cat);
        gtk_tree_selection_unselect_all (sel);
        file_set_current_category (ig->file, NULL);

        gtk_widget_set_sensitive (ig->m_category_previous, TRUE);
        gtk_widget_set_sensitive (ig->m_category_next, FALSE);
        gtk_widget_set_sensitive (ig->t_category_previous, TRUE);
        gtk_widget_set_sensitive (ig->t_category_next, FALSE);

        return TRUE;
    }

    ok = (app_window_find_treev_iter_with_data
        (ig->treev_cat, &iter, CATEGORY(item)) != NULL);

    if (ok)
        app_window_select_treev_iter (ig->treev_cat, &iter);

    return ok;
}


gboolean
app_window_select_treev_root (GtkTreeView *treev)
{
    GtkTreeSelection *sel;
    GtkTreePath *path;
    gboolean ok;

    path = gtk_tree_path_new_first ();

    sel = gtk_tree_view_get_selection (treev);
    gtk_tree_selection_select_path (sel, path);

    ok = gtk_tree_selection_path_is_selected (sel, path);

    if (ok)
        gtk_tree_view_scroll_to_cell (treev, path, NULL, TRUE, 0.5, 0.0);

    gtk_tree_path_free (path);

    return ok;
}


void
app_window_update_expiry_color (Ignuit *ig, GdkColor *color, gboolean redraw)
{
    g_free (ig->color_expiry);
    ig->color_expiry = gdk_color_to_string (color);

    if (redraw) {
        app_window_redraw_card_list (ig);
        app_window_redraw_category_list (ig);
    }
}


static void
app_window_save_geometry (AppWin *d)
{
    gint w, h;

    gtk_window_get_size (GTK_WINDOW(d->window), &w, &h);
    prefs_set_app_size (d->ig->prefs, w, h);

    w = gtk_paned_get_position (GTK_PANED(d->hpaned));
    prefs_set_category_pane_width (d->ig->prefs, w);

    w = gtk_tree_view_column_get_width (d->treev_cat_title_column);
    prefs_set_category_column_title_width (d->ig->prefs, w);

    w = gtk_tree_view_column_get_width (d->treev_card_col[COLUMN_CARD_FRONT]);
    prefs_set_card_column_front_width (d->ig->prefs, w);

    w = gtk_tree_view_column_get_width (d->treev_card_col[COLUMN_CARD_BACK]);
    prefs_set_card_column_back_width (d->ig->prefs, w);

    w = gtk_tree_view_column_get_width (d->treev_card_col[COLUMN_CARD_CATEGORY]);
    prefs_set_card_column_category_width (d->ig->prefs, w);
}


static void
switch_sides (AppWin *d, GtkTreeModel *model, GtkTreeIter *iter,
    gboolean *changed)
{
    GList *item;

    gtk_tree_model_get (model, iter, COLUMN_CARD_DATA, &item, -1);

    card_switch_sides (CARD(item));
    app_window_set_card_iter (d->ig, iter, item);

    *changed = TRUE;
}


static void
toggle_flag (AppWin *d, GtkTreeModel *model, GtkTreeIter *iter,
    gboolean *changed)
{
    gboolean flagged;
    GList *item;

    gtk_tree_model_get (model, iter, COLUMN_CARD_DATA, &item, -1);

    flagged = card_get_flagged (CARD(item));
    card_set_flagged (CARD(item), !flagged);

    app_window_set_card_iter (d->ig, iter, item);

    *changed = TRUE;
}


static void
reset_stats (AppWin *d, GtkTreeModel *model, GtkTreeIter *iter,
    gboolean *changed)
{
    GList *item;

    gtk_tree_model_get (model, iter, COLUMN_CARD_DATA, &item, -1);

    card_reset_statistics (CARD(item));
    app_window_set_card_iter (d->ig, iter, item);

    *changed = TRUE;
}


static void
copy_card (AppWin *d, GtkTreeModel *model, GtkTreeIter *iter,
    gboolean *changed)
{
    GList *item;

    gtk_tree_model_get (model, iter, COLUMN_CARD_DATA, &item, -1);

    ig_add_clipboard (d->ig, card_copy (CARD(item)));
}


static void
remove_card (AppWin *d, GtkTreeModel *model, GtkTreeIter *iter,
    gboolean *changed)
{
    GList *item;
    Category *card_category, *current_category;
    Card *c;


    gtk_tree_model_get (model, iter, COLUMN_CARD_DATA, &item, -1);
    gtk_list_store_remove (GTK_LIST_STORE(model), iter);

    c = CARD(item);
    card_category = card_get_category (c);
    current_category = file_get_current_category (d->ig->file);

    /* Remove it, but don't free it just yet. */
    if (file_category_is_search (d->ig->file, current_category)) {
        file_remove_card (d->ig->file, c);
        category_remove_card (current_category, c);
    }
    else {
        file_remove_card (d->ig->file, c);
    }

    if (file_category_is_trash (d->ig->file, current_category)
        || card_is_blank (c)) {
        /* It's either blank or already in the trash, so delete it. */
        card_free (c);
    }
    else {
        /* Move it to the trash. */
        file_add_trash (d->ig->file, c);
    }

    if (!file_category_is_trash (d->ig->file, card_category))
        app_window_refresh_category_row (d->ig, card_category);

    *changed = TRUE;
}


static void
cut_card (AppWin *d, GtkTreeModel *model, GtkTreeIter *iter,
    gboolean *changed)
{
    GList *item;
    Category *card_category, *current_category;
    Card *c;

    gtk_tree_model_get (model, iter, COLUMN_CARD_DATA, &item, -1);
    gtk_list_store_remove (GTK_LIST_STORE(model), iter);

    c = CARD(item);
    current_category = file_get_current_category (d->ig->file);
    card_category = card_get_category (c);

    /* Remove the card from the file. */
    if (file_category_is_search (d->ig->file, current_category)) {
        file_remove_card (d->ig->file, c);
        category_remove_card (current_category, c);
    }
    else {
        file_remove_card (d->ig->file, c);
    }

    ig_add_clipboard (d->ig, c);

    if (!file_category_is_trash (d->ig->file, card_category))
        app_window_refresh_category_row (d->ig, card_category);

    *changed = TRUE;
}


static gboolean
frobulate_selected_cards (AppWin *d, FrobulationFunc frobfunc,
    gboolean *changed)
{
    GtkTreeRowReference *ref;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GList *rows, *ref_list, *p;


    /* Run frobfunc on each card selected in the card pane. */

    rows = gtk_tree_selection_get_selected_rows (d->sel_cards, &model);
    if (rows == NULL)
        return FALSE;

    /* If editor dialog textviews will be affected, store any edits now. */
    if (frobfunc != toggle_flag
        && frobfunc != reset_stats
        && frobfunc != copy_card) {
        dialog_editor_check_changed ();
    }

    if (frobfunc == copy_card || frobfunc == cut_card)
        ig_clear_clipboard (d->ig);

    ref_list = NULL;

    for (p = rows; p != NULL; p = p->next) {
        path = p->data;
        ref = gtk_tree_row_reference_new (model, path);
        ref_list = g_list_append (ref_list, ref);
    }

    for (p = ref_list; p != NULL; p = p->next) {

        path = gtk_tree_row_reference_get_path (p->data);
        gtk_tree_model_get_iter (model, &iter, path);

        frobfunc (d, model, &iter, changed);

        gtk_tree_path_free (path);

    }

    g_list_foreach (ref_list, (GFunc)gtk_tree_row_reference_free, NULL);
    g_list_free (ref_list);

    g_list_foreach (rows, (GFunc)gtk_tree_path_free, NULL);
    g_list_free (rows);

    if (frobfunc == cut_card || frobfunc == remove_card) {
        if (gtk_list_store_iter_is_valid (GTK_LIST_STORE(model), &iter)) {
            gtk_tree_model_get (model, &iter, COLUMN_CARD_DATA, &p, -1);
            gtk_tree_selection_select_iter (d->sel_cards, &iter);
            file_set_current_item (d->ig->file, p);
        }
        else {

            Category *current_category;

            current_category = file_get_current_category (d->ig->file);

            file_set_current_item (d->ig->file,
                category_get_cards (current_category));

            app_window_select_card (d->ig,
                category_get_cards (current_category));
        }
    }

    if (changed)
        ig_file_changed (d->ig);

    return TRUE;
}


void
error_dialog (GtkWindow *parent, const gchar *message, GError *err)
{
    GtkDialog *dialog;
    gchar *msg;


    if (err && err->message)
        msg = g_strdup_printf ("%s\n\n%s", message, err->message);
    else
        msg = g_strdup (message);

    dialog = (GtkDialog*)gtk_message_dialog_new (parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        NULL);
    gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), msg);

    gtk_dialog_run (dialog);
    gtk_widget_destroy (GTK_WIDGET(dialog));

    g_free (msg);
}


static gboolean
get_save_as_filename (AppWin *d)
{
    GtkWidget *dialog;
    gchar *fname_new;
    const gchar *fname_old;
    gboolean ret = FALSE;
    gint result;


    dialog = gtk_file_chooser_dialog_new (
        _("Save As"), GTK_WINDOW(d->window), GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    fname_old = file_get_filename (d->ig->file);

    if (fname_old != NULL && g_path_is_absolute (fname_old)) {
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(dialog), fname_old);
    }
    else if (prefs_get_workdir (d->ig->prefs)) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),
            prefs_get_workdir (d->ig->prefs));
    }
    result = gtk_dialog_run (GTK_DIALOG(dialog));

    fname_new = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

    if (result == GTK_RESPONSE_ACCEPT && fname_new != NULL) {

        if (g_file_test (fname_new, G_FILE_TEST_EXISTS)) {
            if (!ask_yes_or_no (GTK_WINDOW(d->window),
                _("<b>A file with the same name already exists</b>\n\nDo you want to replace it?"),
                GTK_RESPONSE_NO)) {
                result = GTK_RESPONSE_NONE;
            }
        }

        if (result == GTK_RESPONSE_ACCEPT) {
            file_set_filename (d->ig->file, fname_new);
            ret = TRUE;
        }

        g_free (fname_new);

    }

    gtk_widget_destroy (dialog);

    return ret;
}


static gint
ask_save (GtkWindow *parent)
{
    GtkDialog *dialog;
    gint result;

    dialog = (GtkDialog*)gtk_message_dialog_new_with_markup (parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        _("<b>The current file has not been saved</b>\n\nChanges since last save will be lost."));

    gtk_dialog_add_buttons (dialog,
        _("Close _without Saving"), GTK_RESPONSE_REJECT,
        GTK_STOCK_CANCEL,           GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE,             GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_dialog_set_default_response (dialog, GTK_RESPONSE_ACCEPT);
    result = gtk_dialog_run (dialog);
    gtk_widget_destroy (GTK_WIDGET(dialog));

    if (result != GTK_RESPONSE_ACCEPT && result != GTK_RESPONSE_REJECT)
        result = GTK_RESPONSE_CANCEL;

    return result;
}


gboolean
ask_yes_or_no (GtkWindow *parent, const gchar *question,
    GtkResponseType def)
{
    GtkDialog *dialog;
    gint result;

    dialog = (GtkDialog*)gtk_message_dialog_new (parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        NULL);
    gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), question);

    gtk_dialog_set_default_response (dialog, def);
    result = gtk_dialog_run (dialog);
    gtk_widget_destroy (GTK_WIDGET(dialog));

    return result == GTK_RESPONSE_YES;
}


static void
unable_to_save_file (GtkWindow *parent, GError *err)
{
    error_dialog (parent, _("<b>Unable to save file</b>"), err);

    if (err != NULL)
        g_error_free (err);
}


static gboolean
confirm_empty_trash (AppWin *d)
{
    Category *trash;
    GList *cards;


    if (prefs_get_confirm_empty_trash (d->ig->prefs) == FALSE)
        return TRUE;

    trash = file_get_trash (d->ig->file);
    cards = category_get_cards (trash);

    if (cards == NULL || ask_yes_or_no (GTK_WINDOW(d->window),
        _("<b>Empty the Trash?</b>\n\nRecently removed cards will be permanently deleted."),
            GTK_RESPONSE_NO)) {
        return TRUE;
    }

    return FALSE;
}


static gboolean
save_changes (AppWin *d)
{
    GError *err = NULL;
    gint reply;


    /* If the file has been changed, ask whether or not to save it.
     * Return TRUE only if the changes are either unwanted or
     * successfully saved. */

    if (!file_get_changed (d->ig->file))
        return TRUE;

    if (!confirm_empty_trash (d))
        return FALSE;

    reply = ask_save (GTK_WINDOW(d->window));

    if (reply == GTK_RESPONSE_CANCEL)
        return FALSE;

    if (reply == GTK_RESPONSE_REJECT)
        return TRUE;

    if (!file_get_filename (d->ig->file) && !get_save_as_filename (d))
        return FALSE;

    if (!fileio_save (d->ig, file_get_filename (d->ig->file), &err)) {
        unable_to_save_file (GTK_WINDOW(d->window), err);
        return FALSE;
    }

    app_window_update_title (d->ig);

    return TRUE;
}


static void
cb_m_new (GtkWidget *widget, AppWin *d)
{
    GList *first;

    /* Close the current file and create a new one. */

    dialog_editor_check_changed ();
    dialog_category_properties_close ();

    if (!save_changes (d))
        return;

    file_free (d->ig->file, TRUE);

    d->ig->file = file_new ();
    first = file_add_category (d->ig->file, category_new (NULL));
    file_set_current_category (d->ig->file, CATEGORY(first));

    app_window_refresh_category_pane (d->ig);
    app_window_refresh_card_pane (d->ig,
        file_get_current_category_cards (d->ig->file));

    app_window_update_title (d->ig);
    app_window_update_sensitivity (d);

    dialog_editor_tweak (ED_TWEAK_ALL);
    dialog_properties_tweak ();
}


static void
cb_m_open (GtkWidget *widget, AppWin *d)
{
    GtkWidget *dialog;
    File *file;
    gint result;
    gchar *fname;

    /* Prompt for a file to open, and try to load it. If successful, close
     * the current file and display the newly loaded one. */

    dialog_editor_check_changed ();
    dialog_category_properties_close ();

    if (!save_changes (d))
        return;

    dialog = gtk_file_chooser_dialog_new (_("Open"),
        GTK_WINDOW(d->window), GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    if (prefs_get_workdir (d->ig->prefs)) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),
            prefs_get_workdir (d->ig->prefs));
    }

    result = gtk_dialog_run (GTK_DIALOG(dialog));
    fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy (dialog);

    if (result == GTK_RESPONSE_ACCEPT && fname != NULL) {

        GError *err = NULL;

        if ((file = fileio_load (fname, &err)) != NULL) {

            /* Success - replace the current file with the opened one. */

            GtkTreeModel *model;

            file_set_current_category (file, NULL);
            file_free (d->ig->file, TRUE);
            d->ig->file = file;

            prefs_set_workdir_from_filename (d->ig->prefs,
                file_get_filename (d->ig->file));

            model = gtk_tree_view_get_model (d->ig->treev_cat);

            g_signal_handlers_block_by_func (G_OBJECT(model),
                G_CALLBACK(cb_category_row_inserted), d);

            app_window_refresh_category_pane (d->ig);

            g_signal_handlers_unblock_by_func (G_OBJECT(model),
                G_CALLBACK(cb_category_row_inserted), d);

            if (file_get_categories (d->ig->file)) {
                app_window_select_treev_root (d->ig->treev_cat);
            }
            else {
                app_window_update_sensitivity (d);
                app_window_refresh_card_pane (d->ig,
                    file_get_current_category_cards (d->ig->file));
            }

            app_window_update_title (d->ig);
            dialog_editor_tweak (ED_TWEAK_ALL);
            dialog_properties_tweak ();

            if (err != NULL) {

                /* The file loaded, but not without problems. */

                error_dialog (GTK_WINDOW(d->window),
                    _("<b>Trouble loading file</b>"), err);
                g_error_free (err);
            }

        }
        else {

            error_dialog (GTK_WINDOW(d->window),
                _("<b>Unable to load file</b>"), err);

            if (err != NULL)
                g_error_free (err);

        }
    }
    g_free (fname);
}


static void
cb_m_save (GtkWidget *widget, AppWin *d)
{
    GError *err = NULL;

    dialog_editor_check_changed ();

    if (file_get_filename (d->ig->file) == NULL && !get_save_as_filename (d))
        return;

    if (!fileio_save (d->ig, file_get_filename (d->ig->file), &err)) {
        unable_to_save_file (GTK_WINDOW(d->window), err);
        return;
    }

    prefs_set_workdir_from_filename (d->ig->prefs,
        file_get_filename (d->ig->file));

    app_window_update_title (d->ig);
}


static void
cb_m_save_as (GtkWidget *widget, AppWin *d)
{
    dialog_editor_check_changed ();

    if (get_save_as_filename (d)) {

        GError *err = NULL;

        if (!fileio_save (d->ig, file_get_filename (d->ig->file), &err)) {
            unable_to_save_file (GTK_WINDOW(d->window), err);
            return;
        }
        else {
            prefs_set_workdir_from_filename (d->ig->prefs,
                file_get_filename (d->ig->file));
        }
        app_window_update_title (d->ig);
    }
}


static GList*
populate_import_export_combo_box (AppWin *d, GtkComboBox *combo,
    gboolean is_import)
{
    GList       *list = NULL;
    GDir        *dir;
    const gchar *whichdir;
    gchar       *dname;

    whichdir = is_import ? IMPORT_DIR : EXPORT_DIR;

    dname = gnome_program_locate_file (d->ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, whichdir, TRUE, NULL);

    if (dname == NULL) {
        g_warning ("Can't find directory: %s\n", whichdir);
        return NULL;
    }

    gtk_combo_box_append_text (combo, _("CSV"));
    gtk_combo_box_append_text (combo, _("TSV"));
    gtk_combo_box_append_text (combo, "Ignuit");

    dir = g_dir_open (dname, 0, NULL);
    if (dir) {

        const gchar *entry;

        while ((entry = g_dir_read_name (dir))) {

            gchar *s, *ext;

            s = g_filename_to_utf8 (entry, -1, NULL, NULL, NULL);
            if (s != NULL) {

                gchar *p;

                /* Drop the file extension. */
                if ((ext = strrchr (s, '.')) != NULL)
                    *ext = '\0';

                /* Replace underscores with spaces.*/
                for (p = s; *p != '\0'; p++)
                    if (*p == '_')
                        *p = ' ';

                gtk_combo_box_append_text (combo, s);
                g_free (s);
                list = g_list_append (list, g_strdup (entry));

            }
            else {
                g_warning ("g_filename_to_utf8 failed for '%s'\n", entry);
            }
        }

        g_dir_close (dir);
    }
    g_free (dname);

    return list;
}


static void
cb_m_import (GtkWidget *widget, AppWin *d)
{
    GtkWidget *dialog;
    GtkWidget *box;
    GtkWidget *combo;
    GtkWidget *label;
    GList     *filters;
    File      *file;
    gint      result, combo_selection;
    gchar     *fname;


    /* Import a non-native file, adding it as a new category to the
     * currently open file. */

    dialog = gtk_file_chooser_dialog_new (_("Import"),
        GTK_WINDOW(d->window), GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    if (prefs_get_workdir (d->ig->prefs)) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),
            prefs_get_workdir (d->ig->prefs));
    }

    box = gtk_hbox_new (FALSE, 0);

    combo = gtk_combo_box_new_text ();
    filters = populate_import_export_combo_box (d,
        GTK_COMBO_BOX(combo), TRUE);
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo), 0);

    gtk_box_pack_end (GTK_BOX(box), combo, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("Import _Filter:"));
    gtk_box_pack_end (GTK_BOX(box), label, FALSE, FALSE, 12);

    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER(dialog), box);
    gtk_widget_show_all (box);

    result = gtk_dialog_run (GTK_DIALOG(dialog));
    fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));
    combo_selection = gtk_combo_box_get_active (GTK_COMBO_BOX(combo));
    gtk_widget_destroy (dialog);

    if (result == GTK_RESPONSE_ACCEPT && fname != NULL) {

        GError *err = NULL;

        const gchar *fname_filter;

        switch (combo_selection) {
        case FILTER_CSV:
            file = fileio_import_csv (fname, ',', &err);
            break;
        case FILTER_TSV:
            file = fileio_import_csv (fname, '\t', &err);
            break;
        case FILTER_NATIVE:
            file = fileio_load (fname, &err);
            break;
        default:
            /* >= FILTER_XSLT */
            fname_filter = g_list_nth_data (filters,
                combo_selection - FILTER_XSLT);
            file = fileio_import_xml (d->ig, fname, fname_filter, &err);
            break;
        }

        if (file != NULL && err == NULL) {

            Category *cat, *cat_tmp;
            GList *cur, *item;
            GtkTreeIter iter;

            dialog_editor_check_changed ();

            prefs_set_workdir_from_filename (d->ig->prefs,
                file_get_filename (d->ig->file));

            /* Add cards from the imported file to the currently open file. */

            cat_tmp = file_get_current_category (d->ig->file);

            for (cur = file_get_categories (file); cur != NULL; cur = cur->next) {

                GList *c;

                cat = category_new (category_get_title (CATEGORY(cur)));
                category_set_comment (cat, category_get_comment (CATEGORY(cur)));
                category_set_fixed_order (cat, category_is_fixed_order (CATEGORY(cur)));

                file_add_category (d->ig->file, cat);
                file_set_current_category (d->ig->file, cat);

                for (c = category_get_cards (CATEGORY(cur)); c != NULL; c = c->next)
                    file_add_loaded_card (d->ig->file, CARD(c));

            }

            ig_file_changed (d->ig);

            if (cat_tmp == NULL) {

                GList *categories;

                if ((categories = file_get_categories (d->ig->file)) != NULL)
                    cat_tmp = g_list_nth_data (categories, 0);
            }

            file_set_current_category (d->ig->file, cat_tmp);

            app_window_refresh_category_pane (d->ig);
            app_window_refresh_card_pane (d->ig, category_get_cards (cat));
            app_window_update_appbar (d->ig);

            item = app_window_find_treev_iter_with_data (d->ig->treev_cat,
                &iter, cat_tmp);
            app_window_select_category (d->ig, item);

            /* dialog_editor_tweak (ED_TWEAK_ALL); */
        }
        else {

            error_dialog (GTK_WINDOW(d->window),
                _("<b>Unable to import file</b>"), err);

            if (err != NULL)
                g_error_free (err);

        }

        if (file != NULL)
            file_free (file, FALSE);
    }

    if (filters != NULL) {
        g_list_foreach (filters, (GFunc)g_free, NULL);
        g_list_free (filters);
    }
}


static void
cb_m_export (GtkWidget *widget, AppWin *d)
{
    GtkWidget *dialog;
    GtkWidget *box;
    GtkWidget *combo;
    GtkWidget *label;
    GtkWidget *chk;
    GList     *filters;
    gchar     *fname;
    gint      result, combo_selection;
    gboolean  excl_markup;


    dialog = gtk_file_chooser_dialog_new (
        _("Export"), GTK_WINDOW(d->window), GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    if (prefs_get_workdir (d->ig->prefs)) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),
            prefs_get_workdir (d->ig->prefs));
    }

    box = gtk_hbox_new (FALSE, 0);

    combo = gtk_combo_box_new_text ();
    filters = populate_import_export_combo_box (d,
        GTK_COMBO_BOX(combo), FALSE);
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo), 0);

    gtk_box_pack_end (GTK_BOX(box), combo, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("Export _As:"));
    gtk_box_pack_end (GTK_BOX(box), label, FALSE, FALSE, 12);

    chk = gtk_check_button_new_with_mnemonic (_("Exclude _Markup"));
    gtk_box_pack_end (GTK_BOX(box), chk, FALSE, FALSE, 12);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(chk), FALSE);

    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER(dialog), box);
    gtk_widget_show_all (box);

    result = gtk_dialog_run (GTK_DIALOG(dialog));
    fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    combo_selection = gtk_combo_box_get_active (GTK_COMBO_BOX(combo));
    excl_markup = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(chk));
    gtk_widget_destroy (dialog);

    if (result == GTK_RESPONSE_ACCEPT && fname != NULL) {

        GError *err = NULL;

        const gchar *filter;
        gboolean ok, changed_tmp;

        switch (combo_selection) {
        case FILTER_CSV:
            ok = fileio_export_csv (d->ig->file, fname, ',', excl_markup, &err);
            break;
        case FILTER_TSV:
            ok = fileio_export_csv (d->ig->file, fname, '\t', excl_markup, &err);
            break;
        case FILTER_NATIVE:
            changed_tmp = file_get_changed (d->ig->file);
            ok = fileio_save (d->ig, fname, &err);
            file_set_changed (d->ig->file, changed_tmp);
            break;
        default:
            /* >= FILTER_XSLT */
            filter = g_list_nth_data (filters,
                combo_selection - FILTER_XSLT);
            ok = fileio_export_xml (d->ig, fname, filter, excl_markup, &err);
            break;
        }

        if (!ok) {

            error_dialog (GTK_WINDOW(d->window),
                _("<b>Unable to export file</b>"), err);

            if (err != NULL)
                g_error_free (err);

        }

        g_free (fname);

    }

    if (filters != NULL) {
        g_list_foreach (filters, (GFunc)g_free, NULL);
        g_list_free (filters);
    }
}


static void
cb_m_properties (GtkWidget *widget, AppWin *d)
{
    dialog_properties (d->ig);
}


static void
cb_m_quit (GtkWidget *widget, AppWin *d)
{
    dialog_editor_check_changed ();

    if (!save_changes (d))
        return;

    app_window_save_geometry (d);

    g_free (d);

    gtk_main_quit ();
}


static void
cb_m_add_card (GtkWidget *widget, AppWin *d)
{
    GtkTreeIter iter;
    GList *item;
    Category *cat;
    Card *c;


    /* Make a new blank card, add it to the currently selected category,
     * and open the card editor to fill it in. */

    dialog_editor_check_changed ();

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

    app_window_update_sensitivity (d);

    dialog_editor (d->ig);
}


static void
cb_m_remove_card (GtkWidget *widget, AppWin *d)
{
    gboolean changed = FALSE;

    if (frobulate_selected_cards (d, (FrobulationFunc)remove_card, &changed))
        dialog_editor_tweak (ED_TWEAK_ALL);

    app_window_update_sensitivity (d);
}


static void
cb_m_edit_card (GtkWidget *widget, AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    GList *rows, *item;


    rows = gtk_tree_selection_get_selected_rows (d->sel_cards, &model);
    g_assert (rows != NULL);

    /* Use the first selected row. */
    path = (GtkTreePath*)rows->data;

    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_CARD_DATA, &item, -1);
    app_window_select_treev_iter (d->ig->treev_card, &iter);

    file_set_current_item (d->ig->file, item);

    g_list_foreach (rows, (GFunc)gtk_tree_path_free, NULL);
    g_list_free (rows);

    dialog_editor (d->ig);
}


static void
cb_m_find (GtkWidget *widget, AppWin *d)
{
    dialog_find (d->ig);
}


static void
cb_m_find_simple (GtkWidget *widget, AppWin *d)
{
    Category *cat;
    GList *cur, *cards;

    /* This is used by "Find All", "Find Flagged", and "View Trash". */

    dialog_editor_check_changed ();
    dialog_category_properties_close ();

    file_clear_search (d->ig->file, TRUE);

    if (widget == d->m_find_all) {
        cat = file_get_search (d->ig->file);
        for (cur = file_get_cards (d->ig->file); cur != NULL; cur = cur->next)
            file_add_search_card (d->ig->file, CARD(cur));
    }
    else if (widget == d->m_find_flagged) {
        cat = file_get_search (d->ig->file);
        for (cur = file_get_cards (d->ig->file); cur != NULL; cur = cur->next)
            if (card_get_flagged (CARD(cur)))
                file_add_search_card (d->ig->file, CARD(cur));
    }

    if (widget == d->m_view_trash) {
        cat = file_get_trash (d->ig->file);
    }
    else {
        cat = file_get_search (d->ig->file);
    }

    cards = category_get_cards (cat);

    app_window_refresh_card_pane (d->ig, cards);
    app_window_select_category (d->ig, NULL);
    app_window_select_card (d->ig, cards);

    file_set_current_category (d->ig->file, cat);
    file_set_current_item (d->ig->file, cards);

    app_window_update_sensitivity (d);
    app_window_update_appbar (d->ig);

    dialog_editor_tweak (ED_TWEAK_ALL);
}


static void
cb_m_rename_category (GtkWidget *widget, AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &iter)) {

        GtkTreeViewColumn *col;
        GtkTreePath *path;

        col = gtk_tree_view_get_column (d->ig->treev_cat, 0);
        path = gtk_tree_model_get_path (model, &iter);

        gtk_tree_view_set_cursor (d->ig->treev_cat, path, col, TRUE);

        gtk_tree_path_free (path);
    }
}


static void
cb_m_add_category (GtkWidget *widget, AppWin *d)
{
    GtkTreeIter i_new, i_sel;
    GtkTreeModel *model;
    Category *cat;
    GList *item;


    /* Create a new category and add it to the file. */

    cat = category_new (NULL);
    item = file_add_category (d->ig->file, cat);
    ig_file_changed (d->ig);

    /* Update the category TreeView. Note that this emits row-inserted on
     * the tree model, which results in cb_category_inserted being called. */

    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &i_sel)) {
        gtk_list_store_insert_after (GTK_LIST_STORE(model), &i_new, &i_sel);
    }
    else {
        gtk_tree_model_get_iter_first (model, &i_new);
        gtk_list_store_append (GTK_LIST_STORE(model), &i_new);
    }
    gtk_list_store_set (GTK_LIST_STORE(model), &i_new,
        COLUMN_CATEGORY_DATA, item,
        COLUMN_CATEGORY_TITLE, category_get_title (cat),
        -1);

    /* Switch to the new category. */

    file_set_current_category (d->ig->file, cat);
    file_set_current_item (d->ig->file, NULL);

    app_window_refresh_card_pane (d->ig, NULL);
    app_window_update_sensitivity (d);

    dialog_editor_tweak (ED_TWEAK_ALL);
    dialog_category_properties_tweak ();
}


static void
cb_m_remove_category (GtkWidget *widget, AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *item, *next;
    Category *cat;


    /* Delete the selected category (and any cards it contains). */

    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &iter)) {

        gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);
        if (item == NULL)
            return;

        cat = CATEGORY(item);
        if (category_get_n_cards (cat) && !ask_yes_or_no
            (GTK_WINDOW(d->window),
            _("<b>Delete this non-empty category?</b>\n\nThe category and its contents will be permanently deleted."),
            GTK_RESPONSE_NO)) {
            return;
        }

        dialog_editor_check_changed ();
        dialog_category_properties_close ();

        file_remove_category (d->ig->file, cat);
        ig_file_changed (d->ig);

        if (gtk_list_store_remove (GTK_LIST_STORE(model), &iter)) {
            gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &next, -1);
            gtk_tree_selection_select_iter (d->sel_categories, &iter);
        }
        else {
            if (!app_window_select_treev_root (d->ig->treev_cat)) {

                /* We've just deleted the last available category. */

                file_set_current_category (d->ig->file, NULL);
                file_set_current_item (d->ig->file, NULL);
                app_window_refresh_card_pane (d->ig, NULL);
                dialog_editor_tweak (ED_TWEAK_ALL);
            }
        }

        app_window_update_sensitivity (d);
    }
}


#if 0
static gboolean
current_category_is_first (AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean is_first;

    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &iter)) {

        model = gtk_tree_view_get_model (d->ig->treev_cat);
        path = gtk_tree_model_get_path (model, &iter);

        is_first = !gtk_tree_path_prev (path);

        gtk_tree_path_free (path);

        return is_first;

    }
    return TRUE;
}


static gboolean
current_category_is_last (AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean is_last;

    return FALSE;
    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &iter)) {

        model = gtk_tree_view_get_model (d->ig->treev_cat);
        path = gtk_tree_model_get_path (model, &iter);

        //is_last = !gtk_tree_path_next (path);

        gtk_tree_path_free (path);

        //return is_last;

    }
    return TRUE;
}
#endif


static void
cb_m_category_previous (GtkWidget *widget, AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;

    if (file_current_category_is_special (d->ig->file)) {

        /* If the current category is special (trash, search results),
         * the Previous Category button takes us back to the last
         * selected real category.
         */

        g_assert (d->last_selected_category != NULL);

        app_window_select_category (d->ig, d->last_selected_category);
        return;
    }

    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &iter)) {

        dialog_editor_check_changed ();
        dialog_category_properties_check_changed ();

        model = gtk_tree_view_get_model (d->ig->treev_cat);
        path = gtk_tree_model_get_path (model, &iter);

        gtk_tree_path_prev (path);
        gtk_tree_selection_select_path (d->sel_categories, path);
        gtk_tree_view_scroll_to_cell (d->ig->treev_cat, path, NULL,
            TRUE, 0.5, 0.0);

        gtk_tree_path_free (path);
    }
}


static void
cb_m_category_next (GtkWidget *widget, AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;

    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &iter)) {

        dialog_editor_check_changed ();
        dialog_category_properties_check_changed ();

        model = gtk_tree_view_get_model (d->ig->treev_cat);
        path = gtk_tree_model_get_path (model, &iter);

        gtk_tree_path_next (path);
        gtk_tree_selection_select_path (d->sel_categories, path);
        gtk_tree_view_scroll_to_cell (d->ig->treev_cat, path, NULL,
            TRUE, 0.5, 0.0);

        gtk_tree_path_free (path);
    }
}


static void
cb_m_category_properties (GtkWidget *widget, AppWin *d)
{
    dialog_category_properties (d->ig, d->m_category_popup_toggle_fixed_order);
}


static void
cb_m_category_toggle_fixed_order (GtkWidget *widget, AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (d->sel_categories, &model, &iter)) {

        GList *item, *next;
        Category *cat;
        gboolean fixed;

        gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);
        if (item == NULL) {
            return;
        }

        cat = CATEGORY(item);

        fixed = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(widget));

        category_set_fixed_order (cat, fixed);

        dialog_category_properties_tweak ();

        ig_file_changed (d->ig);
    }
}


static void
cb_m_select_all (GtkWidget *widget, AppWin *d)
{
    GtkTreeSelection *sel;

    sel = gtk_tree_view_get_selection (d->ig->treev_card);
    gtk_tree_selection_select_all (sel);
}


static void
cb_m_flag (GtkWidget *widget, AppWin *d)
{
    gboolean dummy;

    if (frobulate_selected_cards (d, (FrobulationFunc)toggle_flag, &dummy))
        dialog_editor_tweak (ED_TWEAK_UI | ED_TWEAK_TITLE);
}


static void
cb_m_edit_tags (GtkWidget *widget, AppWin *d)
{
    dialog_tagger (d->ig);
}


static void
cb_m_switch_sides (GtkWidget *widget, AppWin *d)
{
    gboolean dummy;

    if (frobulate_selected_cards (d, (FrobulationFunc)switch_sides, &dummy))
        dialog_editor_tweak (ED_TWEAK_ALL);
}


static void
cb_m_reset_stats (GtkWidget *widget, AppWin *d)
{
    gboolean dummy;

    if (!ask_yes_or_no (GTK_WINDOW(d->window),
        _("<b>Expiry dates will be reset</b>\n\nRecorded dates and statistics for the selected cards will be lost. Continue anyway?"),
        GTK_RESPONSE_NO)) {
        return;
    }

    if (frobulate_selected_cards (d, (FrobulationFunc)reset_stats, &dummy))
        dialog_editor_tweak (ED_TWEAK_INFO);
}


static void
cb_m_paste_card (GtkWidget *widget, AppWin *d)
{
    GtkTreeIter iter;
    GList *cur, *item;
    Category *cat, *clipboard;
    Card *c;


    /* Move cards from the clipboard to the currently selected category. */

    clipboard = ig_get_clipboard (d->ig);
    g_assert (clipboard != NULL);

    dialog_editor_check_changed ();

    cat = file_get_current_category (d->ig->file);

    while ((cur = category_get_cards (clipboard))) {

        c = CARD(cur);
        category_remove_card (clipboard, c);
        file_add_card (d->ig->file, cat, c);

        item = category_get_cards (cat);
        file_set_current_item (d->ig->file, item);
        app_window_add_card_iter (d->ig, &iter, item);
        app_window_refresh_category_row (d->ig, cat);

    }

    ig_file_changed (d->ig);
    ig_clear_clipboard (d->ig);
    app_window_update_sensitivity (d);

    dialog_editor_tweak (ED_TWEAK_ALL);
}


static void
cb_m_copy_card (GtkWidget *widget, AppWin *d)
{
    gboolean dummy;

    if (frobulate_selected_cards (d, (FrobulationFunc)copy_card, &dummy)) {

        if (ig_get_clipboard (d->ig) != NULL) {
            gtk_widget_set_sensitive (d->m_paste_card, TRUE);
            gtk_widget_set_sensitive (d->m_card_popup_paste, TRUE);
        }
        app_window_update_sensitivity (d);
    }
}


static void
cb_m_cut_card (GtkWidget *widget, AppWin *d)
{
    gboolean dummy;

    if (frobulate_selected_cards (d, (FrobulationFunc)cut_card, &dummy)) {

        dialog_editor_tweak (ED_TWEAK_ALL);

        if (ig_get_clipboard (d->ig) != NULL) {
            gtk_widget_set_sensitive (d->m_paste_card, TRUE);
            gtk_widget_set_sensitive (d->m_card_popup_paste, TRUE);
        }
        app_window_update_sensitivity (d);
    }
}


static void
cb_m_preferences (GtkWidget *widget, AppWin *d)
{
    dialog_preferences (d->ig);
}


static void
cb_m_help (GtkWidget *widget, AppWin *d)
{
    GError *err = NULL;

    gnome_help_display ("ignuit.xml", NULL, &err);

    if (err != NULL) {
        error_dialog (GTK_WINDOW(d->window),
            _("Couldn't display user guide"), err);
        g_error_free (err);
    }
}


static void
cb_m_view_main_toolbar (GtkCheckMenuItem *widget, AppWin *d)
{
    gboolean visible;

    visible = gtk_check_menu_item_get_active (widget);
    prefs_set_main_toolbar_visible (d->ig->prefs, visible);

    if (visible)
        gtk_widget_show (toolbar);
    else
        gtk_widget_hide (toolbar);
}


static void
cb_m_view_category_pane (GtkCheckMenuItem *widget, AppWin *d)
{
    gboolean visible;

    visible = gtk_check_menu_item_get_active (widget);
    prefs_set_category_pane_visible (d->ig->prefs, visible);

    if (visible)
        gtk_widget_show (category_pane);
    else
        gtk_widget_hide (category_pane);
}


static void
cb_m_view_bottom_toolbar (GtkCheckMenuItem *widget, AppWin *d)
{
    gboolean visible;

    visible = gtk_check_menu_item_get_active (widget);
    prefs_set_bottom_toolbar_visible (d->ig->prefs, visible);

    if (visible) {
        gtk_widget_show (category_buttonbox);
        gtk_widget_show (card_buttonbox);
    }
    else {
        gtk_widget_hide (category_buttonbox);
        gtk_widget_hide (card_buttonbox);
    }
}


static void
cb_m_view_statusbar (GtkCheckMenuItem *widget, AppWin *d)
{
    gboolean visible;

    visible = gtk_check_menu_item_get_active (widget);
    prefs_set_statusbar_visible (d->ig->prefs, visible);

    if (visible)
        gtk_widget_show (appbar);
    else
        gtk_widget_hide (appbar);
}


static gint
get_radio_selection (GSList *group, gint n)
{
    GSList *cur;

    for (cur = group; cur != NULL; cur = cur->next) {

        n--;

        if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(cur->data)))
            return n;
    }

    g_assert_not_reached ();
}


static void
get_quiz_settings (AppWin *d)
{
    dialog_editor_check_changed ();
    dialog_editor_kill ();

    d->ig->quizinfo.category_selection = get_radio_selection
        (d->quiz_category_group, QUIZ_N_CATEGORY_SELECTIONS);
    d->ig->quizinfo.card_selection = get_radio_selection
        (d->quiz_card_group, QUIZ_N_CARD_SELECTIONS);
    d->ig->quizinfo.face_selection = get_radio_selection
        (d->quiz_face_group, QUIZ_N_FACE_SELECTIONS);
}


static void
cb_m_start_quiz (GtkWidget *widget, AppWin *d)
{
    get_quiz_settings (d);
    dialog_quiz (d->ig, QUIZ_MODE_NORMAL);
}


static void
cb_m_start_drill (GtkWidget *widget, AppWin *d)
{
    get_quiz_settings (d);
    dialog_quiz (d->ig, QUIZ_MODE_DRILL);
}


static void
cb_m_quiz_in_order (GtkCheckMenuItem *widget, AppWin *d)
{
    d->ig->quizinfo.in_order = gtk_check_menu_item_get_active (widget);
}


static void
cb_m_about (GtkWidget *widget, AppWin *d)
{
    dialog_about (d->window);
}


static void
update_column_toggle_sensitivity (AppWin *d)
{
    gboolean have_front, have_back;


    /* Make sure we can't hide both the Front _and_ Back column. */

    have_front = prefs_get_card_column_visible (d->ig->prefs,
        COLUMN_CARD_FRONT);
    have_back = prefs_get_card_column_visible (d->ig->prefs,
        COLUMN_CARD_BACK);

    gtk_widget_set_sensitive (d->m_card_header_toggle[COLUMN_CARD_FRONT],
        have_back);

    gtk_widget_set_sensitive (d->m_card_header_toggle[COLUMN_CARD_BACK],
        have_front);
}


static void
cb_m_card_header_toggle (GtkWidget *widget, AppWin *d)
{
    gboolean visible;
    gint i, col = 0;

    for (i = 1; i < CARDS_N_COLUMNS; i++) {
        if (widget == d->m_card_header_toggle[i]) {
            col = i;
            break;
        }
    }

    visible = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(widget));
    gtk_tree_view_column_set_visible (d->treev_card_col[col], visible);
    prefs_set_card_column_visible (d->ig->prefs, col, visible);

    update_column_toggle_sensitivity (d);
}


static gboolean
cb_category_pane_popup (GtkWidget *widget, GdkEventButton *event, AppWin *d)
{
    if (event->type != GDK_BUTTON_PRESS || event->button != 3)
        return FALSE;

    gtk_menu_popup (GTK_MENU(d->popup_menu_category),
        NULL, NULL, NULL, NULL, 3, event->time);

    return TRUE;
}


static gboolean
cb_card_pane_clicked (GtkWidget *widget, GdkEventButton *event, AppWin *d)
{
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    if (event->button == 1) {

        GtkTreeViewColumn *col;
        GtkTreePath *path;

        if (!gtk_tree_view_get_path_at_pos (d->ig->treev_card,
            event->x, event->y, &path, &col, NULL, NULL)) {
            return FALSE;
        }

        if (!strcmp(_("Flag"), gtk_tree_view_column_get_title (col))) {

            GtkTreeIter iter;
            GtkTreeModel *model;
            gboolean changed;

            /* dialog_editor_check_changed (); */

            model = gtk_tree_view_get_model (d->ig->treev_card);
            gtk_tree_model_get_iter (model, &iter, path);

            toggle_flag (d, model, &iter, &changed);

            ig_file_changed (d->ig);
            dialog_editor_tweak (ED_TWEAK_UI | ED_TWEAK_TITLE);

        }

        gtk_tree_path_free (path);

        return FALSE;
    }

    if (event->button == 3) {
        gtk_menu_popup (GTK_MENU(d->popup_menu_card),
            NULL, NULL, NULL, NULL, 3, event->time);
        return TRUE;
    }

    return FALSE;
}


static gboolean
cb_card_header_popup (GtkWidget *widget, GdkEventButton *event, AppWin *d)
{

    if (event->type != GDK_BUTTON_PRESS || event->button != 3)
        return FALSE;

    gtk_menu_popup (GTK_MENU(d->popup_menu_card_header),
        NULL, NULL, NULL, NULL, 3, event->time);

    return TRUE;
}


static void
tweak_editor (AppWin *d)
{
    if (file_get_current_category_cards (d->ig->file) != NULL)
        app_window_select_treev_root (d->ig->treev_card);

    dialog_editor_tweak (ED_TWEAK_ALL);
}


static void
cb_treev_category_selection (GtkTreeSelection *sel, AppWin *d)
{
    GtkTreeIter iter;
    GtkTreeModel *model;


    /* A category has been selected in the category TreeView.
     * Update the card TreeView accordingly. */

    if (gtk_tree_selection_get_selected (sel, &model, &iter)) {

        GtkAdjustment *adj;
        GList *item;
        Category *cat;

        dialog_editor_check_changed ();
        dialog_category_properties_check_changed ();

        gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);
        cat = item ? CATEGORY(item) : NULL;

        if (cat == NULL)
            return;

        d->last_selected_category = item;

        g_signal_handlers_block_by_func (G_OBJECT(d->m_category_popup_toggle_fixed_order),
            G_CALLBACK(cb_m_category_toggle_fixed_order), d);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(d->m_category_popup_toggle_fixed_order),
            category_is_fixed_order (cat));
        g_signal_handlers_unblock_by_func (G_OBJECT(d->m_category_popup_toggle_fixed_order),
            G_CALLBACK(cb_m_category_toggle_fixed_order), d);

        file_set_current_category (d->ig->file, cat);
        file_set_current_item (d->ig->file,
            cat ? category_get_cards (cat) : NULL);

        adj = gtk_scrolled_window_get_vadjustment
            (GTK_SCROLLED_WINDOW(d->scrollw_cards));
        gtk_adjustment_set_value (adj, 0);

        app_window_refresh_card_pane (d->ig,
            file_get_current_category_cards (d->ig->file));

        tweak_editor (d);
        dialog_category_properties_tweak ();

        app_window_update_sensitivity (d);
    }
}


static void
cb_category_row_inserted (GtkTreeModel *tree_model,
    GtkTreePath  *path,
    GtkTreeIter  *iter,
    AppWin       *d)
{
    GtkTreeView *treev;
    GtkTreeSelection *sel;


    /* Select the newly added category. Called after adding a new category
     * or drag'n'dropping an existing row. */

    treev = d->ig->treev_cat;

    sel = gtk_tree_view_get_selection (treev);
    gtk_tree_selection_select_path (sel, path);

    gtk_tree_view_scroll_to_cell (treev, path, NULL, TRUE, 0.5, 0.0);
}


static void
cb_treev_category_title_edited (GtkCellRendererText *cell,
    gchar *path_string, gchar *new_text, AppWin *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;


    /* A category's title has been edited in the category TreeView. */

    model = gtk_tree_view_get_model (d->ig->treev_cat);
    path = gtk_tree_path_new_from_string (path_string);

    if (gtk_tree_model_get_iter (model, &iter, path)) {

        const gchar *old_text;
        GList *item;

        gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);

        old_text = category_get_title (CATEGORY(item));

        if (strcmp (old_text, new_text)) {

            category_set_title (CATEGORY(item), new_text);

            gtk_list_store_set (GTK_LIST_STORE(model), &iter,
                COLUMN_CATEGORY_TITLE, new_text, -1);

            if (prefs_get_card_column_visible (d->ig->prefs,
                    COLUMN_CARD_CATEGORY)) {
                app_window_redraw_card_list (d->ig);
            }

            dialog_category_properties_tweak ();

            ig_file_changed (d->ig);
        }
    }
    gtk_tree_path_free (path);
}


static void
cb_treev_card_activated (GtkTreeView *treev, GtkTreePath *path,
    GtkTreeViewColumn *column, AppWin *d)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    /* A row in the card TreeView has been double-clicked. */

    model = gtk_tree_view_get_model (treev);
    if (gtk_tree_model_get_iter (model, &iter, path)) {

        /* Make this the only selected card. */

        GList *item;

        dialog_editor_check_changed ();

        gtk_tree_selection_unselect_all (d->sel_cards);
        gtk_tree_selection_select_iter (d->sel_cards, &iter);
        gtk_tree_model_get (model, &iter, COLUMN_CARD_DATA, &item, -1);
        file_set_current_item (d->ig->file, item);

        dialog_editor (d->ig);
    }
}


static void
cb_treev_cards_drag_data_get (GtkTreeView *treev,
    GdkDragContext *context, GtkSelectionData *sln_data, guint info,
    guint t, AppWin *d)
{
    GList *cards, *p;

    cards = treev_get_selected_items (d->ig->treev_card);

    for (p = cards; p != NULL; p = p->next)
        d->drag_list = g_list_prepend (d->drag_list, CARD(p));
}


static void
cb_treev_categories_drag_data_received (GtkTreeView *treev,
    GdkDragContext *context,
    gint x, gint y,
    GtkSelectionData *sln_data,
    guint info,
    guint time,
    AppWin *d)
{
    GList *cur, *item, *rowref_list = NULL;
    GtkTreeRowReference *rowref;
    GtkTreePath *path;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Category *cat_dest, *cat_cur;


    /* XXX: This works, but I'm not sure it's the correct way to
     * do drag'n'drop. */

    /* If this is just a category row-reordering, ignore it. */
    if (gtk_drag_get_source_widget (context) ==
        GTK_WIDGET(d->ig->treev_cat)) {
        ig_file_changed (d->ig);
        return;
    }

    /* Get drag destination row. */
    if (!gtk_tree_view_get_dest_row_at_pos (treev, x, y, &path, NULL))
        return;

    model = gtk_tree_view_get_model (treev);
    gtk_tree_model_get_iter (model, &iter, path);

    /* Get drag destination category. */
    gtk_tree_model_get (model, &iter, COLUMN_CATEGORY_DATA, &item, -1);
    cat_dest = CATEGORY(item);

    /* Build a list of row references from the dragged cards. */
    model = gtk_tree_view_get_model (d->ig->treev_card);
    for (cur = d->drag_list; cur != NULL; cur = cur->next) {

        app_window_find_treev_iter_with_data (d->ig->treev_card, &iter,
            CARD(cur));

        path = gtk_tree_model_get_path (model, &iter);
        rowref = gtk_tree_row_reference_new (model, path);
        rowref_list = g_list_append (rowref_list, rowref);

        gtk_tree_path_free (path);
    }


    dialog_editor_check_changed ();


    /* Move the cards to their new category. */

    /* Where the cards are being dragged from. */
    cat_cur = file_get_current_category (d->ig->file);

    for (cur = rowref_list; cur != NULL; cur = cur->next) {

        if ((path = gtk_tree_row_reference_get_path
            ((GtkTreeRowReference*)cur->data)) != NULL) {

            if (gtk_tree_model_get_iter (model, &iter, path)) {

                Card *c;
                Category *cat_src;

                gtk_tree_model_get (model, &iter, COLUMN_CARD_DATA, &item, -1);

                c = CARD(item);
                cat_src = card_get_category (c);

                if (cat_src == cat_dest)
                    continue;

                gtk_list_store_remove (GTK_LIST_STORE(model), &iter);

                /* Unlink the card from the file. */
                if (file_category_is_search (d->ig->file, cat_cur)) {
                    file_remove_card (d->ig->file, c);
                    category_remove_card (cat_cur, c);
                }
                else {
                    file_remove_card (d->ig->file, c);
                }

                /* Add the card to its new category. */
                file_add_card (d->ig->file, cat_dest, c);

                if (!file_category_is_trash (d->ig->file, cat_cur))
                    app_window_refresh_category_row (d->ig, cat_src);

                ig_file_changed (d->ig);
            }

            gtk_tree_path_free (path);
        }
    }

    g_list_foreach (rowref_list, (GFunc)gtk_tree_row_reference_free, NULL);
    g_list_free (rowref_list);

    g_list_free (d->drag_list);
    d->drag_list = NULL;

    if (gtk_list_store_iter_is_valid (GTK_LIST_STORE(model), &iter)) {
        gtk_tree_model_get (model, &iter, COLUMN_CARD_DATA, &item, -1);
        gtk_tree_selection_select_iter (d->sel_cards, &iter);
        file_set_current_item (d->ig->file, item);
    }
    else {
        file_set_current_item (d->ig->file, category_get_cards (cat_cur));
    }

    app_window_refresh_category_row (d->ig, cat_dest);
    app_window_update_sensitivity (d);

    dialog_editor_tweak (ED_TWEAK_ALL);

    gtk_drag_finish (context, FALSE, FALSE, time);
}


static void
cb_treev_cards_selection (GtkTreeSelection *sel, AppWin *d)
{
    GtkTreeModel *model;
    GList *rows;
    gboolean selected;
    gint n_tmp;


    /* The card pane selection has changed. Update options and appbar. */

    rows = gtk_tree_selection_get_selected_rows (sel, &model);
    selected = (rows != NULL);

    gtk_widget_set_sensitive (d->m_edit_card, selected);
    gtk_widget_set_sensitive (d->m_remove_card, selected);
    gtk_widget_set_sensitive (d->m_cut_card, selected);
    gtk_widget_set_sensitive (d->m_copy_card, selected);

    gtk_widget_set_sensitive (d->m_card_popup_edit, selected);
    gtk_widget_set_sensitive (d->m_card_popup_remove, selected);
    gtk_widget_set_sensitive (d->m_card_popup_cut, selected);
    gtk_widget_set_sensitive (d->m_card_popup_copy, selected);

    gtk_widget_set_sensitive (d->b_edit_card, selected);
    gtk_widget_set_sensitive (d->b_remove_card, selected);

    n_tmp = d->ig->n_cards_selected;

    if (rows) {

        d->ig->n_cards_selected = g_list_length (rows);

        g_list_foreach (rows, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (rows);

    }
    else {

        d->ig->n_cards_selected = 0;

    }

    if (d->ig->n_cards_selected != n_tmp)
        app_window_update_appbar (d->ig);

    dialog_tagger_tweak ();
}


static gboolean
cb_poll_expiry (Ignuit *ig)
{
    /* If it's the start of a new hour, check for newly expired cards
     * and update the display. */

    if (get_current_minute () == 0) {
        file_check_expired (ig->file);
        app_window_redraw_card_list (ig);
        app_window_redraw_category_list (ig);
        app_window_update_appbar (ig);
    }
    return TRUE;
}


static void
cb_window_destroy (GtkWidget *widget, AppWin *d)
{
    g_source_remove (d->poll_timeout_id);

    g_free (d);

    gtk_main_quit ();
}


static gboolean
cb_window_delete (GtkWidget *widget, GdkEvent *event, AppWin *d)
{
    dialog_editor_check_changed ();

    if (!save_changes (d))
        return TRUE;

    app_window_save_geometry (d);

    return FALSE;
}


static GtkTreeViewColumn*
add_category_column (AppWin *d, const gchar *title, gint col,
    GtkTreeCellDataFunc func)
{
    GtkCellRenderer *r;
    GtkTreeViewColumn *c;

    /* Add a column to the category TreeView. */

    r = gtk_cell_renderer_text_new ();
    c = gtk_tree_view_column_new_with_attributes (title, r, "text", col, NULL);
    gtk_tree_view_column_set_resizable (GTK_TREE_VIEW_COLUMN(c), TRUE);
    gtk_tree_view_append_column (d->ig->treev_cat, c);

    if (func != NULL)
        gtk_tree_view_column_set_cell_data_func (c, r, func, d, NULL);

    if (col == COLUMN_CATEGORY_TITLE) {
        gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN(c),
            GTK_TREE_VIEW_COLUMN_FIXED);
        g_object_set (r, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        g_object_set (r, "editable", TRUE, NULL);

        g_signal_connect(G_OBJECT(r), "edited",
            G_CALLBACK(cb_treev_category_title_edited), d);

        d->treev_cat_title_column = c;
    }

    return c;
}


static GtkTreeViewColumn*
add_card_column (AppWin *d, const gchar *title, gint col,
    GtkTreeCellDataFunc func)
{
    GtkCellRenderer *r;
    GtkTreeViewColumn *c;


    /* Add a column to the card TreeView. */

    r = gtk_cell_renderer_text_new ();
    c = gtk_tree_view_column_new_with_attributes (title, r, "text", col, NULL);
    gtk_tree_view_column_set_resizable (GTK_TREE_VIEW_COLUMN(c), TRUE);
    gtk_tree_view_append_column (d->ig->treev_card, c);

    if (func != NULL)
        gtk_tree_view_column_set_cell_data_func (c, r, func, d, NULL);

    if (col == COLUMN_CARD_FRONT || col == COLUMN_CARD_BACK ||
        col == COLUMN_CARD_CATEGORY) {
        gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN(c),
            GTK_TREE_VIEW_COLUMN_FIXED);
        g_object_set (r, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    }

    gtk_tree_view_column_set_clickable (c, TRUE);
    g_signal_connect (G_OBJECT(c->button), "button-press-event",
        G_CALLBACK(cb_card_header_popup), d);

    d->treev_card_col[col] = c;

    return c;
}


static void
set_card_column_visibility (AppWin *d)
{
    gint col;
    gboolean visible;

    for (col = 1; col < CARDS_N_COLUMNS; col++) {

        visible = prefs_get_card_column_visible (d->ig->prefs, col);

        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(d->m_card_header_toggle[col]), visible);
        gtk_tree_view_column_set_visible (d->treev_card_col[col], visible);

    }

    update_column_toggle_sensitivity (d);
}


void
app_window (Ignuit *ig)
{
    AppWin *d;
    GtkWidget *m_new, *m_open, *m_save_as, *m_quit;
    GtkWidget *m_import, *m_export;
    GtkWidget *m_add_category, *b_add_category, *m_category_popup_add;
    GtkWidget *m_properties, *m_preferences;
    GtkWidget *m_help, *m_about;
    GtkWidget *t_new, *t_open;
    GtkWidget *w;
    GtkTreeModel *model;
    GtkListStore *list_store;
    gchar *glade_file;
    GladeXML  *glade_xml;
    GdkColor *color;


    glade_file = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_MAIN, TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_MAIN);
        exit (EXIT_FAILURE);
    }

    d = g_new0 (AppWin, 1);

    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;

    d->drag_list = NULL;

    ig->app = glade_xml_get_widget (glade_xml, "app");

    d->window = ig->app;

    gtk_window_set_default_size (GTK_WINDOW(d->window),
        prefs_get_app_width (d->ig->prefs),
        prefs_get_app_height (d->ig->prefs));

    toolbar = glade_xml_get_widget (glade_xml, "toolbar_bonobodockitem");
    category_pane = glade_xml_get_widget (glade_xml, "vbox_category");
    category_buttonbox = glade_xml_get_widget (glade_xml, "category_buttonbox");
    card_buttonbox = glade_xml_get_widget (glade_xml, "card_buttonbox");

    m_new = glade_xml_get_widget (glade_xml, "m_new");
    m_open = glade_xml_get_widget (glade_xml, "m_open");
    d->ig->m_save = glade_xml_get_widget (glade_xml, "m_save");
    m_save_as = glade_xml_get_widget (glade_xml, "m_save_as");
    m_import = glade_xml_get_widget (glade_xml, "m_import");
    m_export = glade_xml_get_widget (glade_xml, "m_export");
    d->m_reset_stats = glade_xml_get_widget (glade_xml, "m_reset_stats");
    m_properties = glade_xml_get_widget (glade_xml, "m_properties");
    m_quit = glade_xml_get_widget (glade_xml, "m_quit");

    d->m_add_card = glade_xml_get_widget (glade_xml, "m_add_card");
    d->m_remove_card = glade_xml_get_widget (glade_xml, "m_remove_card");
    d->m_edit_card = glade_xml_get_widget (glade_xml, "m_edit_card");
    d->m_cut_card = glade_xml_get_widget (glade_xml, "m_cut_card");
    d->m_copy_card = glade_xml_get_widget (glade_xml, "m_copy_card");
    d->m_paste_card = glade_xml_get_widget (glade_xml, "m_paste_card");
    d->m_select_all = glade_xml_get_widget (glade_xml, "m_select_all");
    d->m_find = glade_xml_get_widget (glade_xml, "m_find");
    d->m_find_flagged = glade_xml_get_widget (glade_xml, "m_show_flagged");
    d->m_find_all = glade_xml_get_widget (glade_xml, "m_show_all");
    d->m_view_trash = glade_xml_get_widget (glade_xml, "m_view_trash");
    m_add_category = glade_xml_get_widget (glade_xml, "m_add_category");
    d->m_remove_category = glade_xml_get_widget (glade_xml, "m_remove_category");
    d->m_category_previous = glade_xml_get_widget (glade_xml, "m_category_previous");
    d->m_category_next = glade_xml_get_widget (glade_xml, "m_category_next");
    d->m_edit_tags = glade_xml_get_widget (glade_xml, "m_edit_tags");
    d->m_flag = glade_xml_get_widget (glade_xml, "m_flag");
    d->m_switch_sides = glade_xml_get_widget (glade_xml, "m_switch_sides");
    m_preferences = glade_xml_get_widget (glade_xml, "m_preferences");

    d->m_start_quiz = glade_xml_get_widget (glade_xml, "m_start_quiz");
    d->m_start_drill = glade_xml_get_widget (glade_xml, "m_start_drill");

    d->r_quiz_category_selection = glade_xml_get_widget (glade_xml,
        "r_all_categories");
    d->quiz_category_group = gtk_radio_menu_item_get_group
        (GTK_RADIO_MENU_ITEM(d->r_quiz_category_selection));

    d->r_quiz_card_selection = glade_xml_get_widget
        (glade_xml, "r_all_cards");
    d->quiz_card_group = gtk_radio_menu_item_get_group
        (GTK_RADIO_MENU_ITEM(d->r_quiz_card_selection));

    d->r_quiz_face_selection = glade_xml_get_widget
        (glade_xml, "r_quiz_face_front");
    d->quiz_face_group = gtk_radio_menu_item_get_group
        (GTK_RADIO_MENU_ITEM(d->r_quiz_face_selection));

    d->m_quiz_in_order = glade_xml_get_widget (glade_xml, "m_quiz_in_order");
    g_signal_connect (G_OBJECT(d->m_quiz_in_order), "toggled",
        G_CALLBACK(cb_m_quiz_in_order), d);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(d->m_quiz_in_order),
        FALSE);

    m_help = glade_xml_get_widget (glade_xml, "m_help");
    m_about = glade_xml_get_widget (glade_xml, "m_about");

    t_new = glade_xml_get_widget (glade_xml, "t_new");
    t_open = glade_xml_get_widget (glade_xml, "t_open");
    d->ig->t_save = glade_xml_get_widget (glade_xml, "t_save");
    d->t_find = glade_xml_get_widget (glade_xml, "t_find");
    d->t_category_previous = glade_xml_get_widget (glade_xml, "t_category_previous");
    d->t_category_next = glade_xml_get_widget (glade_xml, "t_category_next");
    d->t_start_quiz = glade_xml_get_widget (glade_xml, "t_start_quiz");
#if 0
    t_help = glade_xml_get_widget (glade_xml, "t_help");
#endif

    b_add_category = glade_xml_get_widget (glade_xml, "b_add_category");
    d->b_remove_category = glade_xml_get_widget
        (glade_xml, "b_remove_category");

    d->b_edit_card = glade_xml_get_widget (glade_xml, "b_edit_card");
    d->b_add_card = glade_xml_get_widget (glade_xml, "b_add_card");
    d->b_remove_card = glade_xml_get_widget (glade_xml, "b_remove_card");

    d->hpaned = glade_xml_get_widget (glade_xml, "hpaned");

    appbar = glade_xml_get_widget (glade_xml, "appbar");
    ig->appbar = (GnomeAppBar*)appbar;



    /* Category list. */

    d->scrollw_categories = glade_xml_get_widget (glade_xml,
        "scrollw_categories");
    ig->treev_cat = GTK_TREE_VIEW(glade_xml_get_widget (glade_xml,
        "treeview_categories"));

    list_store = gtk_list_store_new (CATEGORIES_N_COLUMNS,
        G_TYPE_POINTER,
        G_TYPE_STRING,
        G_TYPE_INT);

    gtk_tree_view_set_model (d->ig->treev_cat,
        GTK_TREE_MODEL(list_store));

    g_object_unref (list_store);

    add_category_column (d, _("Category"), COLUMN_CATEGORY_TITLE, NULL);
    add_category_column (d, NULL, COLUMN_CATEGORY_CARD_COUNT,
        cell_func_category_card_count);

    d->sel_categories = gtk_tree_view_get_selection (d->ig->treev_cat);
    gtk_tree_selection_set_mode (d->sel_categories, GTK_SELECTION_BROWSE);
    gtk_tree_view_set_reorderable (d->ig->treev_cat, TRUE);

    g_signal_connect (G_OBJECT(d->sel_categories), "changed",
        G_CALLBACK (cb_treev_category_selection), d);


    /* Category pane right-click menu. */

    d->popup_menu_category = glade_xml_get_widget (glade_xml,
        "popup_menu_category");

    w = glade_xml_get_widget (glade_xml, "m_category_popup_rename");
    d->m_category_popup_rename = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_rename_category), d);

    w = glade_xml_get_widget (glade_xml, "m_category_popup_add");
    m_category_popup_add = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_add_category), d);

    w = glade_xml_get_widget (glade_xml, "m_category_popup_remove");
    d->m_category_popup_remove = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_remove_category), d);

    w = glade_xml_get_widget (glade_xml, "m_category_properties");
    d->m_category_properties = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_category_properties), d);

    w = glade_xml_get_widget (glade_xml, "m_category_popup_properties");
    d->m_category_popup_properties = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_category_properties), d);

    w = glade_xml_get_widget (glade_xml, "m_category_popup_toggle_fixed_order");
    d->m_category_popup_toggle_fixed_order = w;
    g_signal_connect (G_OBJECT(w), "toggled",
        G_CALLBACK(cb_m_category_toggle_fixed_order), d);


    /* Card pane column headers. */

    d->popup_menu_card_header = glade_xml_get_widget (glade_xml,
        "popup_menu_card_header");

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_front");
    d->m_card_header_toggle[COLUMN_CARD_FRONT] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_back");
    d->m_card_header_toggle[COLUMN_CARD_BACK] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_category");
    d->m_card_header_toggle[COLUMN_CARD_CATEGORY] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_group");
    d->m_card_header_toggle[COLUMN_CARD_GROUP] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_tested");
    d->m_card_header_toggle[COLUMN_CARD_TESTED] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_expired");
    d->m_card_header_toggle[COLUMN_CARD_EXPIRED] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_expiry_date");
    d->m_card_header_toggle[COLUMN_CARD_EXPIRY_DATE] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_expiry_time");
    d->m_card_header_toggle[COLUMN_CARD_EXPIRY_TIME] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);

    w = glade_xml_get_widget (glade_xml, "m_card_header_toggle_flag");
    d->m_card_header_toggle[COLUMN_CARD_FLAGGED] = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_card_header_toggle), d);


    /* Card pane right-click menu. */

    d->popup_menu_card = glade_xml_get_widget (glade_xml,
        "popup_menu_card");

    w = glade_xml_get_widget (glade_xml, "m_card_popup_add");
    d->m_card_popup_add = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_add_card), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_remove");
    d->m_card_popup_remove = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_remove_card), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_edit");
    d->m_card_popup_edit = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_edit_card), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_cut");
    d->m_card_popup_cut = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_cut_card), d);
 
    w = glade_xml_get_widget (glade_xml, "m_card_popup_copy");
    d->m_card_popup_copy = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_copy_card), d);
 
    w = glade_xml_get_widget (glade_xml, "m_card_popup_paste");
    d->m_card_popup_paste = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_paste_card), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_select_all");
    d->m_card_popup_select_all = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_select_all), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_edit_tags");
    d->m_card_popup_edit_tags = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_edit_tags), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_flag");
    d->m_card_popup_flag = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_flag), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_switch_sides");
    d->m_card_popup_switch_sides = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_switch_sides), d);

    w = glade_xml_get_widget (glade_xml, "m_card_popup_reset_stats");
    d->m_card_popup_reset_stats = w;
    g_signal_connect (G_OBJECT(w), "activate",
        G_CALLBACK(cb_m_reset_stats), d);


    /* Card list. */

    d->scrollw_cards = glade_xml_get_widget (glade_xml, "scrollw_cards");
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(d->scrollw_cards),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    ig->treev_card = GTK_TREE_VIEW(glade_xml_get_widget (glade_xml,
        "treeview_cards"));

    list_store = gtk_list_store_new (CARDS_N_COLUMNS,
        G_TYPE_POINTER, /* This card's category GList node. */
        G_TYPE_STRING,  /* Front text. */
        G_TYPE_STRING,  /* Back text. */
        G_TYPE_STRING,  /* Category. */
        G_TYPE_INT,     /* Group. */
        G_TYPE_STRING,  /* Date last tested. */
        G_TYPE_BOOLEAN, /* Expiry flag. */
        G_TYPE_STRING,  /* Date of expiry. */
        G_TYPE_INT,     /* Time of expiry. */
        G_TYPE_BOOLEAN);/* Flagged by user. */

    gtk_tree_view_set_model (d->ig->treev_card, GTK_TREE_MODEL(list_store));
    g_object_unref (list_store);

    add_card_column (d, _("Front"), COLUMN_CARD_FRONT, NULL);
    add_card_column (d, _("Back"), COLUMN_CARD_BACK, NULL);
    add_card_column (d, _("Category"), COLUMN_CARD_CATEGORY, NULL);
    add_card_column (d, _("B") /* Box */, COLUMN_CARD_GROUP, cell_func_card_group);
    add_card_column (d, _("Tested"), COLUMN_CARD_TESTED, NULL);
    add_card_column (d, "!", COLUMN_CARD_EXPIRED, cell_func_card_expired);
    add_card_column (d, _("Expiry"), COLUMN_CARD_EXPIRY_DATE, NULL);
    add_card_column (d, _("Time"), COLUMN_CARD_EXPIRY_TIME, cell_func_card_expiry_time);
    add_card_column (d, _("Flag"), COLUMN_CARD_FLAGGED, cell_func_card_flagged);

    d->sel_cards = gtk_tree_view_get_selection (d->ig->treev_card);
    gtk_tree_selection_set_mode (d->sel_cards, GTK_SELECTION_MULTIPLE);

    g_signal_connect (G_OBJECT(d->sel_cards), "changed",
        G_CALLBACK(cb_treev_cards_selection), d);

    set_card_column_visibility (d);


    /* View menu. */

    w = glade_xml_get_widget (glade_xml, "m_view_main_toolbar");
    g_signal_connect (G_OBJECT(w), "toggled",
        G_CALLBACK(cb_m_view_main_toolbar), d);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(w),
        prefs_get_main_toolbar_visible (d->ig->prefs));

    w = glade_xml_get_widget (glade_xml, "m_view_category_pane");
    g_signal_connect (G_OBJECT(w), "toggled",
        G_CALLBACK(cb_m_view_category_pane), d);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(w),
        prefs_get_category_pane_visible (d->ig->prefs));

    w = glade_xml_get_widget (glade_xml, "m_view_bottom_toolbar");
    g_signal_connect (G_OBJECT(w), "toggled",
        G_CALLBACK(cb_m_view_bottom_toolbar), d);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(w),
        prefs_get_bottom_toolbar_visible (d->ig->prefs));

    w = glade_xml_get_widget (glade_xml, "m_view_statusbar");
    g_signal_connect (G_OBJECT(w), "toggled",
        G_CALLBACK(cb_m_view_statusbar), d);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(w),
        prefs_get_statusbar_visible (d->ig->prefs));


    /* Signals. */

    g_signal_connect (G_OBJECT(d->window), "destroy",
        G_CALLBACK(cb_window_destroy), d);
    g_signal_connect (G_OBJECT(d->window), "delete_event",
        G_CALLBACK(cb_window_delete), d);

    g_signal_connect (G_OBJECT(m_new), "activate",
        G_CALLBACK(cb_m_new), d);
    g_signal_connect (G_OBJECT(m_open), "activate",
        G_CALLBACK(cb_m_open), d);
    g_signal_connect (G_OBJECT(d->ig->m_save), "activate",
        G_CALLBACK(cb_m_save), d);
    g_signal_connect (G_OBJECT(m_save_as), "activate",
        G_CALLBACK(cb_m_save_as), d);
    g_signal_connect (G_OBJECT(m_import), "activate",
        G_CALLBACK(cb_m_import), d);
    g_signal_connect (G_OBJECT(m_export), "activate",
        G_CALLBACK(cb_m_export), d);
    g_signal_connect (G_OBJECT(d->m_reset_stats), "activate",
        G_CALLBACK(cb_m_reset_stats), d);
    g_signal_connect (G_OBJECT(m_properties), "activate",
        G_CALLBACK(cb_m_properties), d);
    g_signal_connect (G_OBJECT(m_quit), "activate",
        G_CALLBACK(cb_m_quit), d);

    g_signal_connect (G_OBJECT(d->m_edit_card), "activate",
        G_CALLBACK(cb_m_edit_card), d);
    g_signal_connect (G_OBJECT(d->m_cut_card), "activate",
        G_CALLBACK(cb_m_cut_card), d);
    g_signal_connect (G_OBJECT(d->m_copy_card), "activate",
        G_CALLBACK(cb_m_copy_card), d);
    g_signal_connect (G_OBJECT(d->m_paste_card), "activate",
        G_CALLBACK(cb_m_paste_card), d);
    g_signal_connect (G_OBJECT(d->m_find), "activate",
        G_CALLBACK(cb_m_find), d);
    g_signal_connect (G_OBJECT(d->m_find_all), "activate",
        G_CALLBACK(cb_m_find_simple), d);
    g_signal_connect (G_OBJECT(d->m_find_flagged), "activate",
        G_CALLBACK(cb_m_find_simple), d);
    g_signal_connect (G_OBJECT(d->m_view_trash), "activate",
        G_CALLBACK(cb_m_find_simple), d);
    g_signal_connect (G_OBJECT(d->m_add_card), "activate",
        G_CALLBACK(cb_m_add_card), d);
    g_signal_connect (G_OBJECT(d->m_remove_card), "activate",
        G_CALLBACK(cb_m_remove_card), d);
    g_signal_connect (G_OBJECT(m_add_category), "activate",
        G_CALLBACK(cb_m_add_category), d);
    g_signal_connect (G_OBJECT(d->m_remove_category), "activate",
        G_CALLBACK(cb_m_remove_category), d);
    g_signal_connect (G_OBJECT(d->m_category_previous), "activate",
        G_CALLBACK(cb_m_category_previous), d);
    g_signal_connect (G_OBJECT(d->m_category_next), "activate",
        G_CALLBACK(cb_m_category_next), d);
    g_signal_connect (G_OBJECT(d->m_select_all), "activate",
        G_CALLBACK(cb_m_select_all), d);
    g_signal_connect (G_OBJECT(d->m_edit_tags), "activate",
        G_CALLBACK(cb_m_edit_tags), d);
    g_signal_connect (G_OBJECT(d->m_flag), "activate",
        G_CALLBACK(cb_m_flag), d);
    g_signal_connect (G_OBJECT(d->m_switch_sides), "activate",
        G_CALLBACK(cb_m_switch_sides), d);
    g_signal_connect (G_OBJECT(m_preferences), "activate",
        G_CALLBACK(cb_m_preferences), d);

    g_signal_connect (G_OBJECT(d->m_start_quiz), "activate",
        G_CALLBACK(cb_m_start_quiz), d);
    g_signal_connect (G_OBJECT(d->m_start_drill), "activate",
        G_CALLBACK(cb_m_start_drill), d);

    g_signal_connect (G_OBJECT(m_help), "activate",
        G_CALLBACK(cb_m_help), d);
    g_signal_connect (G_OBJECT(m_about), "activate",
        G_CALLBACK(cb_m_about), d);

    g_signal_connect (G_OBJECT(t_new), "clicked",
        G_CALLBACK(cb_m_new), d);
    g_signal_connect (G_OBJECT(t_open), "clicked",
        G_CALLBACK(cb_m_open), d);
    g_signal_connect (G_OBJECT(d->ig->t_save), "clicked",
        G_CALLBACK(cb_m_save), d);
    g_signal_connect (G_OBJECT(d->t_find), "clicked",
        G_CALLBACK(cb_m_find), d);
    g_signal_connect (G_OBJECT(d->t_category_previous), "clicked",
        G_CALLBACK(cb_m_category_previous), d);
    g_signal_connect (G_OBJECT(d->t_category_next), "clicked",
        G_CALLBACK(cb_m_category_next), d);
    g_signal_connect (G_OBJECT(d->t_start_quiz), "clicked",
        G_CALLBACK(cb_m_start_quiz), d);
#if 0
    g_signal_connect (G_OBJECT(t_help), "clicked",
        G_CALLBACK(cb_m_help), d);
#endif

    g_signal_connect (G_OBJECT(b_add_category), "clicked",
        G_CALLBACK(cb_m_add_category), d);
    g_signal_connect (G_OBJECT(d->b_remove_category), "clicked",
        G_CALLBACK(cb_m_remove_category), d);

    g_signal_connect (G_OBJECT(d->b_edit_card), "clicked",
        G_CALLBACK(cb_m_edit_card), d);
    g_signal_connect (G_OBJECT(d->b_add_card), "clicked",
        G_CALLBACK(cb_m_add_card), d);
    g_signal_connect (G_OBJECT(d->b_remove_card), "clicked",
        G_CALLBACK(cb_m_remove_card), d);

    model = gtk_tree_view_get_model (d->ig->treev_cat);
    g_signal_connect (G_OBJECT(model), "row-inserted",
        G_CALLBACK(cb_category_row_inserted), d);

    g_signal_connect (G_OBJECT(d->ig->treev_cat), "button-press-event",
        G_CALLBACK(cb_category_pane_popup), d);
    g_signal_connect (G_OBJECT(d->ig->treev_card), "button-press-event",
        G_CALLBACK(cb_card_pane_clicked), d);

    g_signal_connect (G_OBJECT(d->ig->treev_card), "row-activated",
        G_CALLBACK(cb_treev_card_activated), d);


    /* Drag'n'drop. */

    gtk_tree_view_enable_model_drag_source (d->ig->treev_cat,
        GDK_BUTTON1_MASK,
        target_table,
        G_N_ELEMENTS(target_table),
        GDK_ACTION_MOVE);

    gtk_tree_view_enable_model_drag_dest (d->ig->treev_cat,
        target_table,
        G_N_ELEMENTS(target_table),
        GDK_ACTION_MOVE);

    gtk_tree_view_enable_model_drag_source (d->ig->treev_card,
        GDK_BUTTON1_MASK,
        target_table,
        G_N_ELEMENTS(target_table),
        GDK_ACTION_MOVE);

    g_signal_connect (G_OBJECT(d->ig->treev_card), "drag_data_get",
        G_CALLBACK(cb_treev_cards_drag_data_get), d);
    g_signal_connect (G_OBJECT(d->ig->treev_cat), "drag_data_received",
        G_CALLBACK(cb_treev_categories_drag_data_received), d);


    app_window_refresh_category_pane (d->ig);
    app_window_refresh_card_pane (d->ig,
        file_get_current_category_cards (d->ig->file));

    gtk_paned_set_position (GTK_PANED(d->hpaned),
        prefs_get_category_pane_width (d->ig->prefs));

    gtk_tree_view_column_set_fixed_width (d->treev_cat_title_column,
        prefs_get_category_column_title_width (d->ig->prefs));

    gtk_tree_view_column_set_fixed_width
        (d->treev_card_col[COLUMN_CARD_FRONT],
        prefs_get_card_column_front_width (d->ig->prefs));

    gtk_tree_view_column_set_fixed_width
        (d->treev_card_col[COLUMN_CARD_BACK],
        prefs_get_card_column_back_width (d->ig->prefs));

    gtk_tree_view_column_set_fixed_width
        (d->treev_card_col[COLUMN_CARD_CATEGORY],
        prefs_get_card_column_category_width (d->ig->prefs));

    app_window_update_sensitivity (d);

    /* Scroll to first category. */
    app_window_select_treev_root (d->ig->treev_cat);


    ig->m_remove_category = d->m_remove_category;
    ig->b_remove_category = d->b_remove_category;
    ig->m_category_properties = d->m_category_properties;
    ig->m_add_card = d->m_add_card;
    ig->b_add_card = d->b_add_card;
    ig->m_start_quiz = d->m_start_quiz;
    ig->m_start_drill = d->m_start_drill;
    ig->t_start_quiz = d->t_start_quiz;
    ig->m_find = d->m_find;
    ig->t_find = d->t_find;
    ig->m_find_flagged = d->m_find_flagged;
    ig->m_find_all = d->m_find_all;
    ig->m_view_trash = d->m_view_trash;
    ig->m_edit_tags = d->m_edit_tags;
    ig->m_flag = d->m_flag;
    ig->m_switch_sides = d->m_switch_sides;
    ig->m_reset_stats = d->m_reset_stats;
    ig->m_paste_card = d->m_paste_card;
    ig->m_select_all = d->m_select_all;

    ig->m_category_popup_rename = d->m_category_popup_rename;
    ig->m_category_popup_remove = d->m_category_popup_remove;
    ig->m_category_popup_toggle_fixed_order = d->m_category_popup_toggle_fixed_order;

    ig->m_category_previous = d->m_category_previous;
    ig->m_category_next = d->m_category_next;

    ig->t_category_previous = d->t_category_previous;
    ig->t_category_next = d->t_category_next;

    ig->m_category_popup_properties = d->m_category_popup_properties;

    ig->m_card_popup_paste = d->m_card_popup_paste;
    ig->m_card_popup_edit_tags = d->m_card_popup_edit_tags;
    ig->m_card_popup_flag = d->m_card_popup_flag;
    ig->m_card_popup_switch_sides = d->m_card_popup_switch_sides;
    ig->m_card_popup_reset_stats = d->m_card_popup_reset_stats;
    ig->m_card_popup_select_all = d->m_card_popup_select_all;


    color = gtk_widget_get_style (GTK_WIDGET(d->ig->treev_cat))->text;
    d->ig->color_plain = gdk_color_to_string (color);

    app_window_update_expiry_color (d->ig,
        prefs_get_color (d->ig->prefs, COLOR_CARD_EXPIRED), FALSE);


    d->poll_timeout_id = g_timeout_add_seconds
        (60, (GSourceFunc)cb_poll_expiry, ig);

    app_window_update_title (ig);

    gtk_widget_show (d->window);

    g_object_unref (G_OBJECT(glade_xml));


    if (!prefs_get_main_toolbar_visible (d->ig->prefs))
        gtk_widget_hide (toolbar);

    if (!prefs_get_category_pane_visible (d->ig->prefs))
        gtk_widget_hide (category_pane);

    if (!prefs_get_statusbar_visible (d->ig->prefs))
        gtk_widget_hide (appbar);

    if (!prefs_get_bottom_toolbar_visible (d->ig->prefs)) {
        gtk_widget_hide (category_buttonbox);
        gtk_widget_hide (card_buttonbox);
    }
}

