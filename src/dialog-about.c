/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2008, 2009, 2012, 2015 Timothy Richard Musson
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
#include "dialog-about.h"


void
dialog_about (GtkWidget *parent)
{
    const char *authors[] = {
        "Timothy Musson <trmusson@gmail.com>",
        NULL
    };
    const char *documenters[] = {
        "Timothy Musson <trmusson@gmail.com>",
        NULL
    };
    gchar *translators = _("translator-credits");


    gtk_show_about_dialog (GTK_WINDOW(parent),
#if 0
#if GTK_CHECK_VERSION (2, 12, 0)
        "program-name", _("i GNU it"),
#else
        "name", _("i GNU it"),
#endif
#endif
        "authors", authors,
        "comments", _("Educational software for GNOME, following the Leitner flash card system."),
        "copyright", "Copyright \302\251 2016 Timothy Richard Musson <trmusson@gmail.com>",
        "documenters", documenters,
#if 0
        "license", license_text,
        "wrap-license", TRUE,
#endif
        "logo-icon-name", "ignuit",
        "translator-credits", translators,
        "version", VERSION,
        "website", "http://homepages.ihug.co.nz/~trmusson/programs.html",
        NULL);
}

