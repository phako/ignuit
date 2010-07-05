/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2008, 2009, 2012 Timothy Richard Musson
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
#include "dialog-properties.h"


typedef struct {
    const gchar *title;
    const gchar *uri;
} License;


/* XXX: The following might be better as a separate text file. */
static License license[] = {
    { "Public Domain",
        "http://creativecommons.org/licenses/publicdomain/" },
    { "CC Attribution",
        "http://creativecommons.org/licenses/by/3.0/" },
    { "CC Attribution-Share Alike",
        "http://creativecommons.org/licenses/by-sa/3.0/" },
    { "CC Attribution-Noncommercial",
        "http://creativecommons.org/licenses/by-nc/3.0/" },
    { "CC Attribution-Noncommercial-Share Alike",
        "http://creativecommons.org/licenses/by-nc-sa/3.0/" },
    { "GNU General Public License version 3",
        "http://www.gnu.org/licenses/gpl.html" },
    { "GNU Free Documentation License 1.3",
        "http://www.gnu.org/licenses/fdl.html" },
    { "Educational Community License 2.0",
        "http://www.osedu.org/licenses/ECL-2.0/" },
    { "Free Art License 1.3",
        "http://artlibre.org/licence/lal/en" },
    { "Other", NULL },
    { NULL, NULL }
};


typedef struct {

    Ignuit              *ig;
    GtkWidget           *window;
    GtkWidget           *entry_title;
    GtkWidget           *entry_author;
    GtkWidget           *entry_description;
    GtkWidget           *entry_homepage;
    GtkWidget           *combo_license;
    GtkWidget           *entry_license_uri;
    GtkWidget           *combo_style;

    CardStyle           style;

} Dialog;


static Dialog *dialog = NULL;


static void
set_widget_values (Dialog *d)
{
    GtkWidget *entry;

    gtk_entry_set_text (GTK_ENTRY(d->entry_title),
        file_get_title (d->ig->file));
    gtk_entry_set_text (GTK_ENTRY(d->entry_author),
        file_get_author (d->ig->file));
    gtk_entry_set_text (GTK_ENTRY(d->entry_description),
        file_get_description (d->ig->file));
    gtk_entry_set_text (GTK_ENTRY(d->entry_homepage),
        file_get_homepage (d->ig->file));
    gtk_entry_set_text (GTK_ENTRY(d->entry_license_uri),
        file_get_license_uri (d->ig->file));

    entry = gtk_bin_get_child (GTK_BIN(d->combo_license));
    gtk_entry_set_text (GTK_ENTRY(entry), file_get_license (d->ig->file));

    d->style = file_get_card_style (d->ig->file);
    gtk_combo_box_set_active (GTK_COMBO_BOX(d->combo_style), d->style - 1);
}


static void
cb_license (GtkWidget *widget, Dialog *d)
{
    gint item;

    item = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));

    if (item != -1) {

        if (license[item].uri != NULL) {

            gtk_entry_set_text (GTK_ENTRY(d->entry_license_uri),
                license[item].uri ? license[item].uri : "");

        }
        else {

            /* "Other" */

            GtkWidget *entry;

            entry = gtk_bin_get_child (GTK_BIN(d->combo_license));
            gtk_entry_set_text (GTK_ENTRY(entry), "");
            gtk_widget_grab_focus (entry);

        }
    }
}


static void
cb_style (GtkWidget *widget, Dialog *d)
{
    d->style = gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) + 1;
    file_set_card_style (d->ig->file, d->style);
    dialog_editor_card_style_changed ();
    ig_file_changed (d->ig);
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
    GtkWidget *entry;

    entry = gtk_bin_get_child (GTK_BIN(d->combo_license));

    return
        strcmp (file_get_title (d->ig->file),
            gtk_entry_get_text (GTK_ENTRY(d->entry_title))) ||
        strcmp (file_get_author (d->ig->file),
            gtk_entry_get_text (GTK_ENTRY(d->entry_author))) ||
        strcmp (file_get_description (d->ig->file),
            gtk_entry_get_text (GTK_ENTRY(d->entry_description))) ||
        strcmp (file_get_homepage (d->ig->file),
            gtk_entry_get_text (GTK_ENTRY(d->entry_homepage))) ||
        strcmp (file_get_license (d->ig->file),
            gtk_entry_get_text (GTK_ENTRY(entry))) ||
        strcmp (file_get_license_uri (d->ig->file),
            gtk_entry_get_text (GTK_ENTRY(d->entry_license_uri)));
}


static void
cb_revert (GtkWidget *widget, Dialog *d)
{
    set_widget_values (d);
}


static void
cb_close (GtkWidget *widget, Dialog *d)
{
    if (properties_changed (d)) {

        GtkWidget *entry;

        entry = gtk_bin_get_child (GTK_BIN(d->combo_license));

        file_set_title (d->ig->file,
            gtk_entry_get_text (GTK_ENTRY(d->entry_title)));
        file_set_author (d->ig->file,
            gtk_entry_get_text (GTK_ENTRY(d->entry_author)));
        file_set_description (d->ig->file,
            gtk_entry_get_text (GTK_ENTRY(d->entry_description)));
        file_set_homepage (d->ig->file,
            gtk_entry_get_text (GTK_ENTRY(d->entry_homepage)));
        file_set_license (d->ig->file,
            gtk_entry_get_text (GTK_ENTRY(entry)));
        file_set_license_uri (d->ig->file,
            gtk_entry_get_text (GTK_ENTRY(d->entry_license_uri)));

        ig_file_changed (d->ig);
    }

    gtk_widget_destroy (d->window);
}


void
dialog_properties_tweak (void)
{
    if (dialog == NULL)
        return;

    set_widget_values (dialog);
}


void
dialog_properties (Ignuit *ig)
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
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_PROPERTIES, TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_PROPERTIES);
        return;
    }

    dialog = d = g_new0 (Dialog, 1);

    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;

    d->window = glade_xml_get_widget (glade_xml, "dialog");
    d->entry_title = glade_xml_get_widget (glade_xml, "entry_title");
    d->entry_author = glade_xml_get_widget (glade_xml, "entry_author");
    d->entry_description = glade_xml_get_widget (glade_xml, "entry_description");
    d->entry_homepage = glade_xml_get_widget (glade_xml, "entry_homepage");
    d->entry_license_uri = glade_xml_get_widget (glade_xml, "entry_license_uri");
    d->combo_style = glade_xml_get_widget (glade_xml, "combo_style");
    b_revert = glade_xml_get_widget (glade_xml, "b_revert");
    b_close = glade_xml_get_widget (glade_xml, "b_close");

    label = glade_xml_get_widget (glade_xml, "label_title");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_title);
    label = glade_xml_get_widget (glade_xml, "label_author");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_author);
    label = glade_xml_get_widget (glade_xml, "label_description");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_description);
    label = glade_xml_get_widget (glade_xml, "label_homepage");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_homepage);
    label = glade_xml_get_widget (glade_xml, "label_license_uri");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->entry_license_uri);
    label = glade_xml_get_widget (glade_xml, "label_style");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->combo_style);

    hbox = glade_xml_get_widget (glade_xml, "hbox_license");

    d->combo_license = gtk_combo_box_entry_new_text ();
    gtk_box_pack_start (GTK_BOX(hbox), d->combo_license, TRUE, TRUE, 0);
    gtk_widget_show (d->combo_license);

    for (i = 0; license[i].title != NULL; i++)
        gtk_combo_box_append_text (GTK_COMBO_BOX(d->combo_license),
            license[i].title);

    label = glade_xml_get_widget (glade_xml, "label_license");
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), d->combo_license);

    gtk_combo_box_append_text (GTK_COMBO_BOX(d->combo_style),
        _("Centered keywords and text"));
    gtk_combo_box_append_text (GTK_COMBO_BOX(d->combo_style),
        _("Non-centered keywords and text"));
    gtk_combo_box_append_text (GTK_COMBO_BOX(d->combo_style),
        _("Non-centered text only"));

    set_widget_values (d);

    g_signal_connect (G_OBJECT(d->window), "destroy",
        G_CALLBACK(cb_destroy), d);

    g_signal_connect (G_OBJECT(d->combo_license), "changed",
        G_CALLBACK(cb_license), d);

    g_signal_connect (G_OBJECT(d->combo_style), "changed",
        G_CALLBACK(cb_style), d);

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

