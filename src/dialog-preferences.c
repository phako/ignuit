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
#include "file.h"
#include "prefs.h"
#include "app-window.h"
#include "dialog-preferences.h"
#include "dialog-editor.h"


typedef struct {

    Ignuit              *ig;

    GtkWidget           *window;

    GtkWidget           *btn_color_fg;
    GtkWidget           *btn_color_bg;
    GtkWidget           *btn_color_bg_known;
    GtkWidget           *btn_color_bg_unknown;
    GtkWidget           *btn_color_bg_end;
    GtkWidget           *btn_color_expired;

    GtkWidget           *spin_0;
    GtkWidget           *spin_1;
    GtkWidget           *spin_2;
    GtkWidget           *spin_3;
    GtkWidget           *spin_4;
    GtkWidget           *spin_5;
    GtkWidget           *spin_6;
    GtkWidget           *spin_7;
    GtkWidget           *spin_8;

    GtkWidget           *spin_latex_dpi;

} Dialog;


static Dialog *dialog = NULL;


static void
set_color_buttons (Dialog *d)
{
    gtk_color_button_set_color (GTK_COLOR_BUTTON(d->btn_color_fg),
        prefs_get_color (d->ig->prefs, COLOR_CARD_FG));
    gtk_color_button_set_color (GTK_COLOR_BUTTON(d->btn_color_bg),
        prefs_get_color (d->ig->prefs, COLOR_CARD_BG));
    gtk_color_button_set_color (GTK_COLOR_BUTTON(d->btn_color_bg_known),
        prefs_get_color (d->ig->prefs, COLOR_CARD_BG_KNOWN));
    gtk_color_button_set_color (GTK_COLOR_BUTTON(d->btn_color_bg_unknown),
        prefs_get_color (d->ig->prefs, COLOR_CARD_BG_UNKNOWN));
    gtk_color_button_set_color (GTK_COLOR_BUTTON(d->btn_color_bg_end),
        prefs_get_color (d->ig->prefs, COLOR_CARD_BG_END));
    gtk_color_button_set_color (GTK_COLOR_BUTTON(d->btn_color_expired),
        prefs_get_color (d->ig->prefs, COLOR_CARD_EXPIRED));
}


static void
set_spin_buttons (Dialog *d)
{
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_0),
        prefs_get_schedule (d->ig->prefs, GROUP_NEW));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_1),
        prefs_get_schedule (d->ig->prefs, GROUP_1));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_2),
        prefs_get_schedule (d->ig->prefs, GROUP_2));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_3),
        prefs_get_schedule (d->ig->prefs, GROUP_3));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_4),
        prefs_get_schedule (d->ig->prefs, GROUP_4));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_5),
        prefs_get_schedule (d->ig->prefs, GROUP_5));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_6),
        prefs_get_schedule (d->ig->prefs, GROUP_6));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_7),
        prefs_get_schedule (d->ig->prefs, GROUP_7));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_8),
        prefs_get_schedule (d->ig->prefs, GROUP_8));
}


static void
cb_card_font (GtkFontButton *widget, Dialog *d)
{
    prefs_set_card_font (d->ig->prefs, gtk_font_button_get_font_name (widget));

    dialog_editor_preferences_changed ();
}


static void
cb_color_fg (GtkColorButton *widget, Dialog *d)
{
    GdkColor color;

    gtk_color_button_get_color (widget, &color);
    prefs_set_color_gdk (d->ig->prefs, COLOR_CARD_FG, &color);

    dialog_editor_preferences_changed ();
}


static void
cb_color_bg (GtkColorButton *widget, Dialog *d)
{
    GdkColor color;

    gtk_color_button_get_color (widget, &color);
    prefs_set_color_gdk (d->ig->prefs, COLOR_CARD_BG, &color);

    dialog_editor_preferences_changed ();
}


static void
cb_color_bg_known (GtkColorButton *widget, Dialog *d)
{
    GdkColor color;

    gtk_color_button_get_color (widget, &color);
    prefs_set_color_gdk (d->ig->prefs, COLOR_CARD_BG_KNOWN, &color);

    dialog_editor_preferences_changed ();
}


static void
cb_color_bg_unknown (GtkColorButton *widget, Dialog *d)
{
    GdkColor color;

    gtk_color_button_get_color (widget, &color);
    prefs_set_color_gdk (d->ig->prefs, COLOR_CARD_BG_UNKNOWN, &color);

    dialog_editor_preferences_changed ();
}


static void
cb_color_bg_end (GtkColorButton *widget, Dialog *d)
{
    GdkColor color;

    gtk_color_button_get_color (widget, &color);
    prefs_set_color_gdk (d->ig->prefs, COLOR_CARD_BG_END, &color);

    dialog_editor_preferences_changed ();
}


static void
cb_color_expired (GtkColorButton *widget, Dialog *d)
{
    GdkColor color;

    gtk_color_button_get_color (widget, &color);
    prefs_set_color_gdk (d->ig->prefs, COLOR_CARD_EXPIRED, &color);

    app_window_update_expiry_color (d->ig, &color, TRUE);
    dialog_editor_preferences_changed ();
}


static void
cb_restore_colors (GtkWidget *widget, Dialog *d)
{
    prefs_csvstr_to_colors (d->ig->prefs, DEFAULT_CARD_COLORS);
    set_color_buttons (d);

    app_window_update_expiry_color (d->ig,
        prefs_get_color (d->ig->prefs, COLOR_CARD_EXPIRED), TRUE);
    dialog_editor_preferences_changed ();
}


static void
cb_schedule_changed (GtkWidget *widget, Dialog *d)
{
    Group group;
    gint days;

    if (widget == d->spin_0)
        group = GROUP_NEW;
    else if (widget == d->spin_1)
        group = GROUP_1;
    else if (widget == d->spin_2)
        group = GROUP_2;
    else if (widget == d->spin_3)
        group = GROUP_3;
    else if (widget == d->spin_4)
        group = GROUP_4;
    else if (widget == d->spin_5)
        group = GROUP_5;
    else if (widget == d->spin_6)
        group = GROUP_6;
    else if (widget == d->spin_7)
        group = GROUP_7;
    else if (widget == d->spin_8)
        group = GROUP_8;
    else
        g_assert_not_reached();

    days = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));

    prefs_set_schedule (d->ig->prefs, group, days);
}


static void
cb_latex_dpi_changed (GtkWidget *widget, Dialog *d)
{
    gint dpi;

    dpi = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    prefs_set_latex_dpi (d->ig->prefs, dpi);
}


static void
cb_toggle_backup (GtkWidget *widget, Dialog *d)
{
    gboolean active;

    active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    prefs_set_backup (d->ig->prefs, active);
}


static void
cb_restore_schedules (GtkWidget *widget, Dialog *d)
{
    prefs_csvstr_to_schedules (d->ig->prefs, DEFAULT_SCHEDULES);
    set_spin_buttons (d);
}


static void
cb_destroy (GtkWidget *widget, Dialog *d)
{
    g_free (d);
    dialog = NULL;
}


static void
cb_close (GtkWidget *widget, Dialog *d)
{
    gtk_widget_destroy (d->window);
}


void
dialog_preferences (Ignuit *ig)
{
    Dialog      *d;
    GtkWidget   *btn_close, *btn_card_font;
    GtkWidget   *btn_restore_colors, *btn_restore_schedules;
    GtkWidget   *toggle_backup;
    GladeXML    *glade_xml;
    gchar       *glade_file;
    const gchar *font;


    if (dialog != NULL) {
        gtk_window_present (GTK_WINDOW(dialog->window));
        return;
    }

    glade_file = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, F_GLADE_PREFERENCES, TRUE, NULL);

    if (glade_file == NULL) {
        g_warning ("Can't find file: %s\n", F_GLADE_PREFERENCES);
        return;
    }

    dialog = d = g_new0 (Dialog, 1);
    glade_xml = glade_xml_new (glade_file, NULL, NULL);
    g_free (glade_file);

    d->ig = ig;

    d->window = glade_xml_get_widget (glade_xml, "dialog");

    btn_card_font = glade_xml_get_widget (glade_xml, "btn_font_small");
    d->btn_color_fg = glade_xml_get_widget (glade_xml, "btn_color_fg");
    d->btn_color_bg = glade_xml_get_widget (glade_xml, "btn_color_bg");
    d->btn_color_bg_known = glade_xml_get_widget (glade_xml, "btn_color_bg_known");
    d->btn_color_bg_unknown = glade_xml_get_widget (glade_xml, "btn_color_bg_unknown");
    d->btn_color_bg_end = glade_xml_get_widget (glade_xml, "btn_color_bg_end");
    d->btn_color_expired = glade_xml_get_widget (glade_xml, "btn_color_expired");
    btn_restore_colors = glade_xml_get_widget (glade_xml, "btn_restore_colors");

    d->spin_0 = glade_xml_get_widget (glade_xml, "spin_0");
    d->spin_1 = glade_xml_get_widget (glade_xml, "spin_1");
    d->spin_2 = glade_xml_get_widget (glade_xml, "spin_2");
    d->spin_3 = glade_xml_get_widget (glade_xml, "spin_3");
    d->spin_4 = glade_xml_get_widget (glade_xml, "spin_4");
    d->spin_5 = glade_xml_get_widget (glade_xml, "spin_5");
    d->spin_6 = glade_xml_get_widget (glade_xml, "spin_6");
    d->spin_7 = glade_xml_get_widget (glade_xml, "spin_7");
    d->spin_8 = glade_xml_get_widget (glade_xml, "spin_8");
    btn_restore_schedules = glade_xml_get_widget (glade_xml,
        "btn_restore_schedules");

    d->spin_latex_dpi = glade_xml_get_widget (glade_xml, "spin_latex_dpi");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(d->spin_latex_dpi),
        prefs_get_latex_dpi (d->ig->prefs));

    toggle_backup = glade_xml_get_widget (glade_xml, "toggle_backup");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(toggle_backup),
        prefs_get_backup (d->ig->prefs));

    btn_close = glade_xml_get_widget (glade_xml, "btn_close");

    set_color_buttons (d);
    set_spin_buttons (d);

    if ((font = prefs_get_card_font (d->ig->prefs)) != NULL)
        gtk_font_button_set_font_name (GTK_FONT_BUTTON(btn_card_font), font);

    g_signal_connect (G_OBJECT(d->window), "destroy",
        G_CALLBACK(cb_destroy), d);

    g_signal_connect (G_OBJECT(btn_card_font), "font-set",
        G_CALLBACK(cb_card_font), d);
    g_signal_connect (G_OBJECT(d->btn_color_fg), "color-set",
        G_CALLBACK(cb_color_fg), d);
    g_signal_connect (G_OBJECT(d->btn_color_expired), "color-set",
        G_CALLBACK(cb_color_expired), d);
    g_signal_connect (G_OBJECT(d->btn_color_bg), "color-set",
        G_CALLBACK(cb_color_bg), d);
    g_signal_connect (G_OBJECT(d->btn_color_bg_known), "color-set",
        G_CALLBACK(cb_color_bg_known), d);
    g_signal_connect (G_OBJECT(d->btn_color_bg_unknown), "color-set",
        G_CALLBACK(cb_color_bg_unknown), d);
    g_signal_connect (G_OBJECT(d->btn_color_bg_end), "color-set",
        G_CALLBACK(cb_color_bg_end), d);

    g_signal_connect (G_OBJECT(btn_restore_colors), "clicked",
        G_CALLBACK(cb_restore_colors), d);

    g_signal_connect (G_OBJECT(d->spin_0), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_1), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_2), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_3), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_4), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_5), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_6), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_7), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);
    g_signal_connect (G_OBJECT(d->spin_8), "value-changed",
        G_CALLBACK(cb_schedule_changed), d);

    g_signal_connect (G_OBJECT(d->spin_latex_dpi), "value-changed",
        G_CALLBACK(cb_latex_dpi_changed), d);
    g_signal_connect (G_OBJECT(toggle_backup), "toggled",
        G_CALLBACK(cb_toggle_backup), d);

    g_signal_connect (G_OBJECT(btn_restore_schedules), "clicked",
        G_CALLBACK(cb_restore_schedules), d);

    g_signal_connect (G_OBJECT(btn_close), "clicked",
        G_CALLBACK(cb_close), d);


    gtk_window_set_transient_for (GTK_WINDOW(d->window),
        GTK_WINDOW(ig->app));
    gtk_window_set_modal (GTK_WINDOW(d->window), FALSE);

    gtk_widget_show_all (d->window);

    g_object_unref (G_OBJECT(glade_xml));
}

