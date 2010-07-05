/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2009 Timothy Richard Musson
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
#include "dialog-tagger.h"


enum {
    COL_REMOVE = 0,
    COL_TAGNAME,
    N_COLS
};


typedef struct {

    Ignuit              *ig;

    GList               *cards;
    GList               *tags;

    GtkWidget           *window;
    GtkWidget           *entry_add;
    GtkWidget           *b_add;
    GtkWidget           *b_remove;
    GtkTreeView         *treev;

} Dialog;


static Dialog *dialog = NULL;


static GList*
list_used_tags (GList *cards)
{
    GList *p, *q, *tlist = NULL;
    gboolean dummy;


    /* Return a list of all tags used by these cards. */

    for (p = cards; p != NULL; p = p->next) {
        for (q = card_get_tags (CARD(p)); q != NULL; q = q->next) {
            tlist = tag_list_add_tag (tlist, TAG(q), &dummy);
        }
    }

    return tlist;
}


static void
update_list_store (Dialog *d, GtkListStore *store)
{
    GtkTreeIter iter;
    GList *p;


    /* Update the store and display. */

    for (p = d->tags; p != NULL; p = p->next) {

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
            COL_REMOVE, FALSE,
            COL_TAGNAME, tag_get_name (TAG(p)),
            -1);
    }
}


static void
update_ui (Dialog *d, gboolean file_changed)
{
    GtkTreeModel *model;


    /* Refresh the tag manager's tag list. */

    tag_list_free (d->tags);
    d->tags = list_used_tags (d->cards);

    model = gtk_tree_view_get_model (d->treev);
    gtk_list_store_clear (GTK_LIST_STORE(model));
    update_list_store (d, GTK_LIST_STORE(model));


    /* Update relevant parts of the main and editor windows. */

    if (file_changed) {
        ig_file_changed (d->ig);
        dialog_editor_tweak (ED_TWEAK_TAG_BAR);
    }
}


static void
cb_b_add (GtkWidget *widget, Dialog *d)
{
    GList *p;
    const gchar *s;
    gchar **strv;
    gboolean changed = FALSE;


    /* Add any new tags to the selected cards. */

    s = gtk_entry_get_text (GTK_ENTRY(d->entry_add));
    strv = g_strsplit (s, " ", 0);

    for (p = d->cards; p != NULL; p = p->next)
        changed |= file_card_add_new_tags_from_strv (d->ig->file,
            CARD(p), strv);

    g_strfreev (strv);

    gtk_entry_set_text (GTK_ENTRY(d->entry_add), "");

    if (changed)
        update_ui (d, TRUE);
}


static void
cb_b_remove (GtkWidget *widget, Dialog *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *p, *q, *tlist = NULL;
    gboolean valid, remove, changed = FALSE;


    model = gtk_tree_view_get_model (d->treev);
    valid = gtk_tree_model_get_iter_first (model, &iter);

    /* Make a list of tags marked for removal. */

    while (valid) {

        const gchar *name;

        gtk_tree_model_get (model, &iter,
            COL_REMOVE, &remove,
            COL_TAGNAME, &name,
            -1);

        if (remove) {

            gboolean dummy;
            GList *item;

            item = tag_list_lookup_tag_by_name (file_get_tags (d->ig->file),
                name);
            tlist = tag_list_add_tag (tlist, TAG(item), &dummy);
        }
    
        valid = gtk_tree_model_iter_next (model, &iter);
    }


    /* Remove those tags from the selected cards. */

    for (p = d->cards; p != NULL; p = p->next)
        for (q = tlist; q != NULL; q = q->next)
            changed |= file_card_remove_tag (d->ig->file, CARD(p), TAG(q));

    tag_list_free (tlist);


    if (changed)
        update_ui (d, TRUE);
}


static void
cb_tag_removal_toggled (GtkCellRendererToggle *renderer, gchar *pathstr,
    Dialog *d)
{
    GtkTreeModel *model;
    GtkTreeIter  iter;
    GtkTreePath  *path;
    gboolean     remove;


    /* A checkbutton has been toggled - update the list store/display. */

    model = gtk_tree_view_get_model (d->treev);
    path = gtk_tree_path_new_from_string (pathstr);
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COL_REMOVE, &remove, -1);

    remove = !remove;

    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        COL_REMOVE, remove, -1);

    gtk_tree_path_free (path);
}


static void
cb_tag_name_edited (GtkCellRendererText *cell, gchar *pathstr,
    gchar *str, Dialog *d)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;


    model = gtk_tree_view_get_model (d->treev);
    path = gtk_tree_path_new_from_string (pathstr);

    if (gtk_tree_model_get_iter (model, &iter, path)) {

        const gchar *name;
        gchar **strv, *new_name;

        strv = g_strsplit (str, " ", 2);
        new_name = *strv;

        if (new_name == NULL) {
            g_strfreev (strv);
            gtk_tree_path_free (path);
            return;
        }

        gtk_tree_model_get (model, &iter, COL_TAGNAME, &name, -1);

        if (strcmp (new_name, name) != 0) {

            if (file_rename_tag (d->ig->file, name, new_name)) {
                gtk_list_store_set (GTK_LIST_STORE(model), &iter,
                    COL_TAGNAME, new_name, -1);
                update_ui (d, TRUE);
            }
            else {
                error_dialog (GTK_WINDOW(d->window),
                    _("<b>Tag name not available</b>\n\nSorry, that name is already used by another tag."),
                    NULL);
            }
        }
        g_strfreev (strv);
    }

    gtk_tree_path_free (path);
}


static void
window_close (Dialog *d)
{
    gint w, h;

    gtk_window_get_size (GTK_WINDOW(d->window), &w, &h);
    prefs_set_tagger_size (d->ig->prefs, w, h);

    if (d->cards != NULL)
        g_list_free (d->cards);

    tag_list_free (d->tags);

    /* Free any completely unused tags on the way out. */
    file_delete_unused_tags (d->ig->file);

    gtk_widget_destroy (d->window);

    g_free (d);
    dialog = NULL;
}


static void
cb_window_delete (GtkWidget *widget, GdkEvent *event, Dialog *d)
{
    window_close (d);
}


static void
cb_close (GtkWidget *widget, Dialog *d)
{
    window_close (d);
}


void
dialog_tagger_tweak (void)
{
    GtkTreeModel *model;
    gboolean have_cards;


    /* Update the dialog with currently selected cards. */

    if (dialog == NULL)
        return;

    model = gtk_tree_view_get_model (dialog->treev);
    gtk_list_store_clear (GTK_LIST_STORE(model));

    if (dialog->cards != NULL)
        g_list_free (dialog->cards);

    tag_list_free (dialog->tags);

    dialog->cards = treev_get_selected_items (dialog->ig->treev_card);
    dialog->tags = list_used_tags (dialog->cards);

    have_cards = (dialog->cards != NULL);

    if (have_cards)
        update_list_store (dialog, GTK_LIST_STORE(model));

    gtk_widget_set_sensitive (dialog->entry_add, have_cards);
    gtk_widget_set_sensitive (dialog->b_add, have_cards);
    gtk_widget_set_sensitive (dialog->b_remove, have_cards);
}


void
dialog_tagger (Ignuit *ig)
{
    Dialog    *d;
    GtkListStore *store;
    GtkWidget *scroll;
    GtkWidget *b_close;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GladeXML  *glade_xml;
    gchar     *glade_file;


    if (dialog != NULL) {
        gtk_window_present (GTK_WINDOW(dialog->window));
        return;
    }

    glade_file = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_TAGGER, TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_TAGGER);
        return;
    }

    dialog = d = g_new0 (Dialog, 1);

    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;

    d->window = glade_xml_get_widget (glade_xml, "dialog");

    gtk_window_set_default_size (GTK_WINDOW(d->window),
        prefs_get_tagger_width (d->ig->prefs),
        prefs_get_tagger_height (d->ig->prefs));

    d->entry_add = glade_xml_get_widget (glade_xml, "entry_add");
    d->b_add = glade_xml_get_widget (glade_xml, "b_add");
    d->b_remove = glade_xml_get_widget (glade_xml, "b_remove");
    b_close = glade_xml_get_widget (glade_xml, "b_close");

    scroll = glade_xml_get_widget (glade_xml, "scroll_remove");
    d->treev = GTK_TREE_VIEW(glade_xml_get_widget (glade_xml, "treev_remove"));

    store = gtk_list_store_new (N_COLS,
        G_TYPE_BOOLEAN,
        G_TYPE_STRING);

    gtk_tree_view_set_model (d->treev, GTK_TREE_MODEL(store));

    g_object_unref (store);

    renderer = gtk_cell_renderer_toggle_new ();
    g_signal_connect (renderer, "toggled",
        G_CALLBACK (cb_tag_removal_toggled), d);

    column = gtk_tree_view_column_new_with_attributes (NULL,
        renderer,
        "active", COL_REMOVE,
        NULL);
    gtk_tree_view_append_column (d->treev, column);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited",
        G_CALLBACK(cb_tag_name_edited), d);

    column = gtk_tree_view_column_new_with_attributes (NULL,
        renderer,
        "text", COL_TAGNAME,
        NULL);
    gtk_tree_view_append_column (d->treev, column);

    gtk_tree_view_set_search_column (d->treev, COL_TAGNAME);


    g_signal_connect (G_OBJECT(d->window), "delete-event",
        G_CALLBACK(cb_window_delete), d);
    g_signal_connect (G_OBJECT(d->entry_add), "activate",
        G_CALLBACK(cb_b_add), d);
    g_signal_connect (G_OBJECT(d->b_add), "clicked",
        G_CALLBACK(cb_b_add), d);
    g_signal_connect (G_OBJECT(d->b_remove), "clicked",
        G_CALLBACK(cb_b_remove), d);
    g_signal_connect (G_OBJECT(b_close), "clicked",
        G_CALLBACK(cb_close), d);


    gtk_window_set_transient_for (GTK_WINDOW(d->window),
        GTK_WINDOW(ig->app));

    /* Fill in the tree view. */
    dialog_tagger_tweak ();

    gtk_widget_show_all (d->window);


    g_object_unref (G_OBJECT(glade_xml));
}

