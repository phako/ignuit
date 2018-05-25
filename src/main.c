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
#include <locale.h>
#include <gnome.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>
#include <signal.h>

#include "main.h"
#include "file.h"
#include "fileio.h"
#include "app-window.h"
#include "latex.h"


Ignuit *ignuit_for_signal = NULL;


static void 
signal_handler (int signum)
{
    const gchar *filename;
    GError *err = NULL;

    g_warning ("Received signal %d.\n", signum);
    if (signum != SIGINT && signum != SIGTERM && signum != SIGHUP) {
        g_warning ("Signal %d is not meant to be handled manually.\n", signum);
        return;
    }

    if (ignuit_for_signal == NULL || ignuit_for_signal->file == NULL
        || file_get_changed (ignuit_for_signal->file) == FALSE) {
        exit (1);
    }

    filename = file_get_filename (ignuit_for_signal->file);
    if (filename == NULL)
        exit (1);

    if (!fileio_save (ignuit_for_signal, filename, &err)) {
        if (err != NULL) {
            g_printerr ("Saving '%s' failed: %s\n", filename, err->message);
            g_error_free (err);
            exit (1);
        }
        else {
            g_printerr ("Saving '%s' failed for unknown reasons.\n", filename);
            exit (1);
        }
    }
    exit (1);
}


static void 
install_signal_handler (int signum, struct sigaction *new_action)
{
    struct sigaction old_action;

    if (sigaction (signum, NULL, &old_action) == -1) {
        g_warning ("Unable to retrieve signal handler for signal %d.\n", signum);
        return;
    }

    if (old_action.sa_handler == SIG_IGN) {
        g_warning ("Signal %d is configured to be ignored; refusing to install custom signal handler.", signum);
        return;
    }

    if (sigaction(signum, new_action, NULL) == -1) {
        g_warning ("Unable to install custom signal handler for signal %d.\n", signum);
    }
}


static void 
install_signal_handlers (void)
{
    struct sigaction new_action;

    new_action.sa_handler = signal_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    /* Block the signals we handle during handler execution to avoid
     * race conditions. */

    sigaddset (&new_action.sa_mask, SIGINT);
    sigaddset (&new_action.sa_mask, SIGTERM);
    sigaddset (&new_action.sa_mask, SIGHUP);

    install_signal_handler (SIGINT, &new_action);
    install_signal_handler (SIGTERM, &new_action);
    install_signal_handler (SIGHUP, &new_action);
}


gint
get_current_hour (void)
{
    time_t t;
    struct tm *lt;

    /* Return the number of hours past midnight (0..23). */

    if (time (&t) == -1 || (lt = localtime (&t)) == NULL)
        return 0;

    return lt->tm_hour;
}


gint
get_current_minute (void)
{
    time_t t;
    struct tm *lt;

    /* Return the number of minutes past the hour (0..59). */

    if (time (&t) == -1 || (lt = localtime (&t)) == NULL)
        return -1;

    return lt->tm_min;
}


gboolean
date_today (GDate *date)
{
    /* Fill an existing GDate with today's date. */

    time_t t;

    if (time (&t) == -1)
        return FALSE;

    g_date_set_time_t (date, t);

    return TRUE;
}


gchar*
date_str (gchar *dest, GDate *date)
{
    /* Fill dest with the date in YYYY-MM-DD format, '\0' terminated.
     * The dest string must be already allocated to at least 11 chars. */

    if (date == NULL) {
        dest[0] = '-';
        dest[1] = '\0';
        return dest;
    }

    g_snprintf (dest, 11, "%04d-%02d-%02d",
        g_date_get_year (date),
        g_date_get_month (date),
        g_date_get_day (date));

    return dest;
}


void
ignuit_free (Ignuit *ig)
{
    file_free (ig->file, TRUE);
    prefs_free (ig->prefs);

    g_rand_free (ig->grand);
    g_date_free (ig->today);

    if (ig->recent_search_terms) {
        g_list_foreach (ig->recent_search_terms, (GFunc)g_free, NULL);
        g_list_free (ig->recent_search_terms);
    }

    ig_clear_clipboard (ig);

    g_free (ig->color_expiry);
    g_free (ig->color_plain);

    g_free (ig);
}


void
ig_clear_clipboard (Ignuit *ig)
{
    if (ig->clipboard) {
        category_free (ig->clipboard, TRUE);
        ig->clipboard = NULL;
    }
}


void
ig_add_clipboard (Ignuit *ig, Card *c)
{
    if (ig->clipboard == NULL)
        ig->clipboard = category_new ("");

    category_prepend_card (ig->clipboard, c);
}


Category*
ig_get_clipboard (Ignuit *ig)
{
    return ig->clipboard;
}


gboolean
ig_category_is_clipboard (Ignuit *ig, Category *cat)
{
    return (cat == ig->clipboard);
}


void
ig_file_changed (Ignuit *ig)
{
    /* Let the user know that there are unsaved changes. */

    if (file_get_changed (ig->file) == FALSE) {
        file_set_changed (ig->file, TRUE);
        app_window_update_title (ig);
    }
}


int
main (int argc, char *argv[])
{
    GOptionContext *context;
    GOptionGroup *gstreamer_group;
    GError *err = NULL;
    Ignuit *ig;


    setlocale (LC_ALL, "");

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

#if 0
    /* g_thread_init has been deprecated since version 2.32 and should not
       be used in newly-written code. This function is no longer necessary. */
    if (!g_thread_supported ())
        g_thread_init (NULL);
#endif

    context = g_option_context_new (PACKAGE);
    gstreamer_group = gst_init_get_option_group ();
    g_option_context_add_group (context, gstreamer_group);

    ig = g_new0 (Ignuit, 1);

    ig->program = gnome_program_init (PACKAGE, VERSION,
        LIBGNOMEUI_MODULE, argc, argv,
        GNOME_PROGRAM_STANDARD_PROPERTIES,
        GNOME_PARAM_GOPTION_CONTEXT, context,
        NULL);

    gconf_init (argc, argv, NULL);

    ig->today = g_date_new ();
    ig->grand = g_rand_new ();
    ig->prefs = prefs_load ();

    if (argc > 2) {
        g_printerr ("Usage: %s [OPTIONS] [FILENAME]\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    if (argc > 1) {
        ig->file = fileio_load (argv[1], &err);
        if (ig->file == NULL) {
            g_warning ("%s\n", err->message);
            g_error_free (err);
        }
        else {
            prefs_set_workdir_from_filename (ig->prefs, argv[1]);
        }
    }

    if (ig->file == NULL) {
        ig->file = file_new ();
        file_add_category (ig->file, category_new (NULL));
    }

    if (file_get_categories (ig->file)) {
        Category *first;
        first = CATEGORY(file_get_categories (ig->file));
        file_set_current_category (ig->file, first);
    }

    latex_init ();

    gst_init (&argc, &argv);

    gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
        DATADIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "icons");

    gtk_window_set_default_icon_name ("ignuit");

    app_window (ig);

    ignuit_for_signal = ig;
    install_signal_handlers();
    gtk_main ();

    ignuit_for_signal = NULL;
    ignuit_free (ig);

    latex_free ();

    return EXIT_SUCCESS;
}

