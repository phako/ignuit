/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2016 Timothy Richard Musson
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
#include "dialog-category-properties.h"


typedef struct {

    Ignuit              *ig;
    GtkWidget           *window;
    GtkWidget           *entry_title;
    GtkWidget           *entry_comment;
    GtkWidget           *checkbox_fixed;

    GtkWidget           *m_checkbox_fixed;

} Dialog;


static Dialog *dialog = NULL;


static void
set_widget_values (Dialog *d)
{
    GtkWidget *entry;
    Category *cat;

    g_assert (file_current_category_is_special (d->ig->file) != TRUE);

    cat = file_get_current_category (d->ig->file);
    g_assert (cat != NULL);

    gtk_entry_set_text (GTK_ENTRY(d->entry_title),
        category_get_title (cat));
    gtk_entry_set_text (GTK_ENTRY(d->entry_comment),
        category_get_comment (cat));
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(d->checkbox_fixed), category_is_fixed_order (cat));
}


static void
cb_destroy (GtkWidget *widget, Dialog *d)
{
    g_free (d);
    dialog = NULL;
}


static gboolean
properties_changed (Dialog *d)
{
    Category *cat;
    gboolean fixed;

    cat = file_get_current_category (d->ig->file);

    fixed = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(d->checkbox_fixed));

    return
        strcmp (category_get_title (cat), gtk_entry_get_text (GTK_ENTRY(d->entry_title))) ||
        strcmp (category_get_comment (cat), gtk_entry_get_text (GTK_ENTRY(d->entry_comment))) ||
        fixed != category_is_fixed_order (cat);
}


static void
cb_revert (GtkWidget *widget, Dialog *d)
{
    set_widget_values (d);
}


static void
update_category (Dialog *d)
{
    Category *cat;

    cat = file_get_current_category (d->ig->file);

    category_set_title (cat,
        gtk_entry_get_text (GTK_ENTRY(d->entry_title)));
    category_set_comment (cat,
        gtk_entry_get_text (GTK_ENTRY(d->entry_comment)));
    category_set_fixed_order (cat, gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(d->checkbox_fixed)));

    app_window_refresh_category_row (d->ig, cat);

    ig_file_changed (d->ig);
}


static void
cb_close (GtkWidget *widget, Dialog *d)
{
    if (properties_changed (d))
        update_category (d);

    gtk_widget_destroy (d->window);
}


static void
cb_toggle_fixed_order (GtkWidget *widget, Dialog *d)
{
    gboolean fixed;

    fixed = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(d->m_checkbox_fixed),
        fixed);
}


void
dialog_category_properties_tweak (void)
{
    if (dialog == NULL)
        return;

    set_widget_values (dialog);
}


void
dialog_category_properties_check_changed (void)
{
    if (dialog == NULL)
        return;

    if (properties_changed (dialog))
        update_category (dialog);
}


void
dialog_category_properties_close (void)
{
    if (dialog != NULL)
        cb_close (NULL, dialog);
}


void
dialog_category_properties (Ignuit *ig, GtkWidget *m_checkbox_fixed)
{
    Dialog    *d;
    GtkWidget *label, *hbox;
    GtkWidget *b_close, *b_revert;
    GladeXML  *glade_xml;
    gchar     *glade_file;
    gint i;


    if (dialog != NULL) {
        gtk_window_present (GTK_WINDOW(dialog->window));
        return;
    }

    glade_file = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_CATEGORY_PROPERTIES,
        TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_CATEGORY_PROPERTIES);
        return;
    }

    dialog = d = g_new0 (Dialog, 1);

    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;

    d->window = glade_xml_get_widget (glade_xml, "dialog");
    d->entry_title = glade_xml_get_widget (glade_xml, "entry_title");
    d->entry_comment = glade_xml_get_widget (glade_xml, "entry_comment");
    d->checkbox_fixed = glade_xml_get_widget (glade_xml, "checkbutton");

    d->m_checkbox_fixed = m_checkbox_fixed;

    b_revert = glade_xml_get_widget (glade_xml, "b_revert");
    b_close = glade_xml_get_widget (glade_xml, "b_close");

    label = glade_xml_get_widget (glade_xml, "label_title");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_title);
    label = glade_xml_get_widget (glade_xml, "label_comment");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_comment);

    set_widget_values (d);

    g_signal_connect (G_OBJECT(d->window), "destroy",
        G_CALLBACK(cb_destroy), d);

    g_signal_connect (G_OBJECT(d->checkbox_fixed), "toggled",
        G_CALLBACK(cb_toggle_fixed_order), d);

    g_signal_connect (G_OBJECT(b_revert), "clicked",
        G_CALLBACK(cb_revert), d);
    g_signal_connect (G_OBJECT(b_close), "clicked",
        G_CALLBACK(cb_close), d);


    gtk_window_set_transient_for (GTK_WINDOW(d->window),
        GTK_WINDOW(ig->app));
    gtk_window_set_modal (GTK_WINDOW(d->window), FALSE);

    gtk_widget_show_all (d->window);

    g_object_unref (G_OBJECT(glade_xml));
}

