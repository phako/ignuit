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
#include <gconf/gconf-client.h>

#include "main.h"
#include "prefs.h"
#include "app-window.h" /* CARDS_N_COLUMNS */


struct _Prefs {

    GConfClient *gconf_client;

    gint        app_width;
    gint        app_height;
    gint        quiz_width;
    gint        quiz_height;
    gint        tagger_width;
    gint        tagger_height;
    gint        category_pane_width;
    gint        category_column_title_width;
    gint        card_column_front_width;
    gint        card_column_back_width;
    gint        card_column_category_width;
    gint        latex_dpi;
    AutoListen  listen;
    gboolean    backup;
    gboolean    card_column_visible[CARDS_N_COLUMNS];
    gboolean    sticky_flips;
    gboolean    quiz_answer_bar_visible;
    gboolean    editor_tag_bar_visible;
    gboolean    main_toolbar_visible;
    gboolean    category_pane_visible;
    gboolean    bottom_toolbar_visible;
    gboolean    statusbar_visible;
    gboolean    find_with_regex;
    gboolean    confirm_empty_trash;
    gchar       *card_font;
    gchar       *workdir;
    GdkColor    *color[N_CARD_COLORS];
    gint        schedule[N_GROUPS];

};


static gchar *card_column_name[] = {
    NULL,
    "front",
    "back",
    "category",
    "group",
    "tested",
    "expired",
    "expiry",
    "time",
    "flagged"
};


#ifndef gdk_color_to_string

/* gdk_color_to_string exists in gdk >= 2.12 */

#define gdk_color_to_string(c)  color_to_string(c)

static gchar*
color_to_string (GdkColor* color)
{
    return g_strdup_printf ("#%04X%04X%04X",
        color->red, color->green, color->blue);
}
#endif


void
prefs_csvstr_to_schedules (Prefs *p, const gchar *csvstr)
{
    gchar **v;
    gint i;


    /* Set schedules from a string of comma separated values. */

    v = g_strsplit (csvstr, ",", N_GROUPS);

    for (i = 0; i < N_GROUPS; i++) {

        if (v[i] == NULL)
            break;

        prefs_set_schedule (p, i, atoi (v[i]));

    }
    g_strfreev (v);
}


static gchar*
prefs_schedules_to_csvstr (Prefs *p)
{
    gchar *s, *v[N_GROUPS + 1];
    gint i;


    /* Return a newly allocated string of comma separated schedules. */

    for (i = 0; i < N_GROUPS; i++)
        v[i] = g_strdup_printf ("%d", p->schedule[i]);

    v[N_GROUPS] = NULL;

    s = g_strjoinv (",", v);

    for (i = 0; i < N_GROUPS; i++)
        g_free (v[i]);

    return s;
}


static gchar*
get_default_color (gint i)
{
    gchar **v, *ret;

    v = g_strsplit (DEFAULT_CARD_COLORS, ",", N_CARD_COLORS);
    ret = g_strdup (v[i]);
    g_strfreev (v);
    return ret;
}


void
prefs_csvstr_to_colors (Prefs *p, const gchar *csvstr)
{
    gchar **v;
    gint i;


    /* Set card colours from a string of comma separated values. */

    v = g_strsplit (csvstr, ",", N_CARD_COLORS);
    for (i = 0; i < N_CARD_COLORS; i++) {
        if (v[i] == NULL) {
#if 1
            gchar *s = get_default_color (i);
            prefs_set_color (p, i, s);
            g_free (s);
#else
            break;
#endif
        }
        else {
            prefs_set_color (p, i, v[i]);
        }
    }
    g_strfreev (v);
}


static gchar*
prefs_colors_to_csvstr (Prefs *p)
{
    gchar *s, *v[N_CARD_COLORS + 1];
    gint i;


    /* Return a newly allocated string of comma separated colour values. */

    for (i = 0; i < N_CARD_COLORS; i++) {
        g_assert (p->color[i] != NULL);
        v[i] = gdk_color_to_string (p->color[i]);
    }
    v[N_CARD_COLORS] = NULL;

    s = g_strjoinv (",", v);

    for (i = 0; i < N_CARD_COLORS; i++)
        g_free (v[i]);

    return s;
}


static gchar*
prefs_card_columns_to_csvstr (Prefs *p)
{
    GString *gstr;
    gchar *s;
    gint col;


    /* Return a newly allocated string of comma separated values. */

    gstr = g_string_new ("");

    for (col = 1; col < CARDS_N_COLUMNS; col++) {

        if (p->card_column_visible[col]) {

            if (gstr->len)
                g_string_append_c (gstr, ',');

            g_string_append (gstr, card_column_name[col]);
        }

    }

    s = gstr->str;
    g_string_free (gstr, FALSE);

    return s;
}


void
prefs_csvstr_to_card_columns (Prefs *p, const gchar *csvstr)
{
    gchar **v;
    gint i, col;
    gboolean ok;


    /* Set card column visibility from a string of comma separated values. */

    for (col = 1; col < CARDS_N_COLUMNS; col++)
        p->card_column_visible[col] = FALSE;

    v = g_strsplit (csvstr, ",", CARDS_N_COLUMNS);
    i = 0;

    while (v[i] != NULL) {

        for (col = 1; col < CARDS_N_COLUMNS; col++)
            if (strcmp (v[i], card_column_name[col]) == 0)
                p->card_column_visible[col] = TRUE;

        i++;
    }

    g_strfreev (v);


    /* Make sure at least one column is visible. */

    ok = FALSE;
    col = 1;
    while (col < CARDS_N_COLUMNS && !ok) {
        ok = p->card_column_visible[col];
        col++;
    }

    if (!ok)
        p->card_column_visible[COLUMN_CARD_FRONT] = TRUE;
}


void
prefs_set_backup (Prefs *p, gboolean backup)
{
    if (backup != p->backup) {
        p->backup = backup;
        gconf_client_set_bool (p->gconf_client,
            PREF_BACKUP, backup, NULL);
    }
}


gboolean
prefs_get_backup (Prefs *p)
{
    return p->backup;
}


void
prefs_set_sticky_flips (Prefs *p, gboolean sticky)
{
    if (sticky != p->sticky_flips) {
        p->sticky_flips = sticky;
        gconf_client_set_bool (p->gconf_client,
            PREF_STICKY_FLIPS, sticky, NULL);
    }
}


gboolean
prefs_get_sticky_flips (Prefs *p)
{
    return p->sticky_flips;
}


void
prefs_set_auto_listen (Prefs *p, AutoListen listen)
{
    if (listen != p->listen) {
        p->listen = listen;
        gconf_client_set_int (p->gconf_client, PREF_AUTO_LISTEN, listen, NULL);
    }
}


AutoListen
prefs_get_auto_listen (Prefs *p)
{
    return p->listen;
}


void
prefs_set_category_pane_width (Prefs *p, gint width)
{
    if (width <= 0)
        width = DEFAULT_CATEGORY_PANE_WIDTH;

    if (width != p->category_pane_width) {
        p->category_pane_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_CATEGORY_PANE_WIDTH, width, NULL);
    }
}


gint
prefs_get_category_pane_width (Prefs *p)
{
    return p->category_pane_width;
}


void
prefs_set_main_toolbar_visible (Prefs *p, gboolean visible)
{
    if (visible != p->main_toolbar_visible) {
        p->main_toolbar_visible = visible;
        gconf_client_set_bool (p->gconf_client,
            PREF_MAIN_TOOLBAR_VISIBLE, visible, NULL);
    }
}


gboolean
prefs_get_main_toolbar_visible (Prefs *p)
{
    return p->main_toolbar_visible;
}


void
prefs_set_category_pane_visible (Prefs *p, gboolean visible)
{
    if (visible != p->category_pane_visible) {
        p->category_pane_visible = visible;
        gconf_client_set_bool (p->gconf_client,
            PREF_CATEGORY_PANE_VISIBLE, visible, NULL);
    }
}


gboolean
prefs_get_category_pane_visible (Prefs *p)
{
    return p->category_pane_visible;
}


void
prefs_set_bottom_toolbar_visible (Prefs *p, gboolean visible)
{
    if (visible != p->bottom_toolbar_visible) {
        p->bottom_toolbar_visible = visible;
        gconf_client_set_bool (p->gconf_client,
            PREF_BOTTOM_TOOLBAR_VISIBLE, visible, NULL);
    }
}


gboolean
prefs_get_bottom_toolbar_visible (Prefs *p)
{
    return p->bottom_toolbar_visible;
}


void
prefs_set_statusbar_visible (Prefs *p, gboolean visible)
{
    if (visible != p->statusbar_visible) {
        p->statusbar_visible = visible;
        gconf_client_set_bool (p->gconf_client,
            PREF_STATUSBAR_VISIBLE, visible, NULL);
    }
}


gboolean
prefs_get_statusbar_visible (Prefs *p)
{
    return p->statusbar_visible;
}


void
prefs_set_quiz_answer_bar_visible (Prefs *p, gboolean visible)
{
    if (visible != p->quiz_answer_bar_visible) {
        p->quiz_answer_bar_visible = visible;
        gconf_client_set_bool (p->gconf_client,
            PREF_QUIZ_ANSWERBAR_VISIBLE, visible, NULL);
    }
}


gboolean
prefs_get_quiz_answer_bar_visible (Prefs *p)
{
    return p->quiz_answer_bar_visible;
}


void
prefs_set_editor_tag_bar_visible (Prefs *p, gboolean visible)
{
    if (visible != p->editor_tag_bar_visible) {
        p->editor_tag_bar_visible = visible;
        gconf_client_set_bool (p->gconf_client,
            PREF_EDITOR_TAGBAR_VISIBLE, visible, NULL);
    }
}


gboolean
prefs_get_editor_tag_bar_visible (Prefs *p)
{
    return p->editor_tag_bar_visible;
}


void
prefs_set_color_gdk (Prefs *p, Color which, GdkColor *color)
{
    gboolean changed = FALSE;

    if (p->color[which]) {
        changed = !gdk_color_equal (color, p->color[which]);
        gdk_color_free (p->color[which]);
    }

    p->color[which] = gdk_color_copy (color);

    if (changed) {

        gchar *spec;

        spec = prefs_colors_to_csvstr (p);
        gconf_client_set_string (p->gconf_client, PREF_CARD_COLORS, spec, NULL);
        g_free (spec);
    }
}


void
prefs_set_color (Prefs *p, Color which, const gchar *spec)
{
    GdkColor color;

    if (gdk_color_parse (spec, &color))
        prefs_set_color_gdk (p, which, &color);
}


GdkColor*
prefs_get_color (Prefs *p, Color which)
{
    return p->color[which];
}


Group
prefs_get_schedule (Prefs *p, Group g)
{
    return p->schedule[g];
}


void
prefs_set_schedule (Prefs *p, Group g, gint days)
{
    if (days != p->schedule[g]) {

        gchar *s;

        p->schedule[g] = days;

        s = prefs_schedules_to_csvstr (p);
        gconf_client_set_string (p->gconf_client, PREF_SCHEDULES, s, NULL);
        g_free (s);
    }
}


void
prefs_set_app_size (Prefs *p, gint width, gint height)
{
    if (width < 1)
        width = DEFAULT_APP_WIDTH;

    if (height < 1)
        height = DEFAULT_APP_HEIGHT;

    if (width != p->app_width) {
        p->app_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_APP_WIDTH, width, NULL);
    }

    if (height != p->app_height) {
        p->app_height = height;
        gconf_client_set_int (p->gconf_client,
            PREF_APP_HEIGHT, height, NULL);
    }
}


gint
prefs_get_app_width (Prefs *p)
{
    return p->app_width;
}


gint
prefs_get_app_height (Prefs *p)
{
    return p->app_height;
}


void
prefs_set_quiz_size (Prefs *p, gint width, gint height)
{
    if (width < 1)
        width = DEFAULT_QUIZ_WIDTH;

    if (height < 1)
        height = DEFAULT_QUIZ_HEIGHT;

    if (width != p->quiz_width) {
        p->quiz_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_QUIZ_WIDTH, width, NULL);
    }

    if (height != p->quiz_height) {
        p->quiz_height = height;
        gconf_client_set_int (p->gconf_client,
            PREF_QUIZ_HEIGHT, height, NULL);
    }
}


gint
prefs_get_quiz_width (Prefs *p)
{
    return p->quiz_width;
}


gint
prefs_get_quiz_height (Prefs *p)
{
    return p->quiz_height;
}


void
prefs_set_tagger_size (Prefs *p, gint width, gint height)
{
    if (height < 1)
        height = DEFAULT_TAGGER_HEIGHT;

    if (width < 1)
        width = DEFAULT_TAGGER_WIDTH;

    if (width != p->tagger_width) {
        p->tagger_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_TAGGER_WIDTH, width, NULL);
    }

    if (height != p->tagger_height) {
        p->tagger_height = height;
        gconf_client_set_int (p->gconf_client,
            PREF_TAGGER_HEIGHT, height, NULL);
    }
}


gint
prefs_get_tagger_width (Prefs *p)
{
    return p->tagger_width;
}


gint
prefs_get_tagger_height (Prefs *p)
{
    return p->tagger_height;
}


void
prefs_set_category_column_title_width (Prefs *p, gint width)
{
    if (width <= 0)
        width = DEFAULT_CATEGORY_COLUMN_TITLE_WIDTH;

    if (width != p->category_column_title_width) {
        p->category_column_title_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_CATEGORY_COLUMN_TITLE_WIDTH, width, NULL);
    }
}


gint
prefs_get_category_column_title_width (Prefs *p)
{
    return p->category_column_title_width;
}


void
prefs_set_card_column_front_width (Prefs *p, gint width)
{
    if (width <= 0)
        width = DEFAULT_CARD_COLUMN_FRONT_WIDTH;

    if (width != p->card_column_front_width) {
        p->card_column_front_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_CARD_COLUMN_FRONT_WIDTH, width, NULL);
    }
}


gint
prefs_get_card_column_front_width (Prefs *p)
{
    return p->card_column_front_width;
}


void
prefs_set_card_column_back_width (Prefs *p, gint width)
{
    if (width <= 0)
        width = DEFAULT_CARD_COLUMN_BACK_WIDTH;

    if (width != p->card_column_back_width) {
        p->card_column_back_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_CARD_COLUMN_BACK_WIDTH, width, NULL);
    }
}


gint
prefs_get_card_column_back_width (Prefs *p)
{
    return p->card_column_back_width;
}


void
prefs_set_card_column_category_width (Prefs *p, gint width)
{
    if (width <= 0)
        width = DEFAULT_CARD_COLUMN_CATEGORY_WIDTH;

    if (width != p->card_column_category_width) {
        p->card_column_category_width = width;
        gconf_client_set_int (p->gconf_client,
            PREF_CARD_COLUMN_CATEGORY_WIDTH, width, NULL);
    }
}


gint
prefs_get_card_column_category_width (Prefs *p)
{
    return p->card_column_category_width;
}


void
prefs_set_card_column_visible (Prefs *p, gint col, gboolean visible)
{
    if (visible != p->card_column_visible[col]) {

        gchar *s;

        p->card_column_visible[col] = visible;
        s = prefs_card_columns_to_csvstr (p);

        gconf_client_set_string (p->gconf_client,
            PREF_CARD_COLUMN_VISIBLE, s, NULL);

        g_free (s);
    }
}


gboolean
prefs_get_card_column_visible (Prefs *p, gint col)
{
    return p->card_column_visible[col];
}


void
prefs_set_card_font (Prefs *p, const gchar *fontname)
{
    if (p->card_font == NULL
        || (g_ascii_strcasecmp (p->card_font, fontname) != 0)) {

            g_free (p->card_font);
            p->card_font = g_strdup (fontname);

            gconf_client_set_string (p->gconf_client,
                PREF_CARD_FONT, fontname, NULL);
    }
}


const gchar*
prefs_get_card_font (Prefs *p)
{
    return p->card_font;
}


void
prefs_set_workdir (Prefs *p, const gchar *dirname)
{
    if (p->workdir == NULL || (strcmp (p->workdir, dirname) != 0)) {

        g_free (p->workdir);
        p->workdir = g_strdup (dirname);

        gconf_client_set_string (p->gconf_client, PREF_WORKDIR, dirname, NULL);
    }
}


void
prefs_set_workdir_from_filename (Prefs *p, const gchar *fname)
{
    if (fname != NULL && g_path_is_absolute (fname)) {

        gchar *workdir;

        workdir = g_path_get_dirname (fname);
        prefs_set_workdir (p, workdir);
        g_free (workdir);
    }
}


const gchar*
prefs_get_workdir (Prefs *p)
{
    return p->workdir;
}


void
prefs_set_find_with_regex (Prefs *p, gboolean use_regex)
{
    if (use_regex != p->find_with_regex) {
        p->find_with_regex = use_regex;
        gconf_client_set_bool (p->gconf_client,
            PREF_FIND_WITH_REGEX, use_regex, NULL);
    }
}


gboolean
prefs_get_find_with_regex (Prefs *p)
{
    return p->find_with_regex;
}


void
prefs_set_confirm_empty_trash (Prefs *p, gboolean confirm)
{
    if (confirm != p->confirm_empty_trash) {
        p->confirm_empty_trash = confirm;
        gconf_client_set_bool (p->gconf_client,
            PREF_CONFIRM_EMPTY_TRASH, confirm, NULL);
    }
}


gboolean
prefs_get_confirm_empty_trash (Prefs *p)
{
    return p->confirm_empty_trash;
}


void
prefs_set_latex_dpi (Prefs *p, gint dpi)
{
    if (dpi < 0 || dpi > MAX_LATEX_DPI)
        dpi = DEFAULT_LATEX_DPI;

    if (dpi != p->latex_dpi) {
        p->latex_dpi = dpi;
        gconf_client_set_int (p->gconf_client,
            PREF_LATEX_DPI, dpi, NULL);
    }
}


gint
prefs_get_latex_dpi (Prefs *p)
{
    return p->latex_dpi;
}


Prefs*
prefs_load (void)
{
    gint i, w, h;
    gboolean b;
    gchar *s;
    Prefs *p;

    p = g_new0(Prefs, 1);

    p->gconf_client = gconf_client_get_default ();

    w = gconf_client_get_int (p->gconf_client, PREF_APP_WIDTH, NULL);
    h = gconf_client_get_int (p->gconf_client, PREF_APP_HEIGHT, NULL);
    prefs_set_app_size (p, w, h);

    w = gconf_client_get_int (p->gconf_client, PREF_CATEGORY_PANE_WIDTH, NULL);
    prefs_set_category_pane_width (p, w);

    w = gconf_client_get_int (p->gconf_client, PREF_CATEGORY_COLUMN_TITLE_WIDTH, NULL);
    prefs_set_category_column_title_width (p, w);

    w = gconf_client_get_int (p->gconf_client, PREF_CARD_COLUMN_FRONT_WIDTH, NULL);
    prefs_set_card_column_front_width (p, w);

    w = gconf_client_get_int (p->gconf_client, PREF_CARD_COLUMN_BACK_WIDTH, NULL);
    prefs_set_card_column_back_width (p, w);

    w = gconf_client_get_int (p->gconf_client, PREF_CARD_COLUMN_CATEGORY_WIDTH, NULL);
    prefs_set_card_column_category_width (p, w);

    s = gconf_client_get_string (p->gconf_client, PREF_CARD_COLUMN_VISIBLE, NULL);
    prefs_csvstr_to_card_columns (p, s ? s : DEFAULT_CARD_COLUMN_VISIBLE);
    g_free (s);

    b = gconf_client_get_bool (p->gconf_client, PREF_STICKY_FLIPS, NULL);
    prefs_set_sticky_flips (p, b);

    i = gconf_client_get_int (p->gconf_client, PREF_AUTO_LISTEN, NULL);
    prefs_set_auto_listen (p, i);

    b = gconf_client_get_bool (p->gconf_client, PREF_MAIN_TOOLBAR_VISIBLE, NULL);
    prefs_set_main_toolbar_visible (p, b);

    b = gconf_client_get_bool (p->gconf_client, PREF_CATEGORY_PANE_VISIBLE, NULL);
    prefs_set_category_pane_visible (p, b);

    b = gconf_client_get_bool (p->gconf_client, PREF_BOTTOM_TOOLBAR_VISIBLE, NULL);
    prefs_set_bottom_toolbar_visible (p, b);

    b = gconf_client_get_bool (p->gconf_client, PREF_STATUSBAR_VISIBLE, NULL);
    prefs_set_statusbar_visible (p, b);

    b = gconf_client_get_bool (p->gconf_client, PREF_QUIZ_ANSWERBAR_VISIBLE, NULL);
    prefs_set_quiz_answer_bar_visible (p, b);

    b = gconf_client_get_bool (p->gconf_client, PREF_EDITOR_TAGBAR_VISIBLE, NULL);
    prefs_set_editor_tag_bar_visible (p, b);

    w = gconf_client_get_int (p->gconf_client, PREF_QUIZ_WIDTH, NULL);
    h = gconf_client_get_int (p->gconf_client, PREF_QUIZ_HEIGHT, NULL);
    prefs_set_quiz_size (p, w, h);

    w = gconf_client_get_int (p->gconf_client, PREF_TAGGER_WIDTH, NULL);
    h = gconf_client_get_int (p->gconf_client, PREF_TAGGER_HEIGHT, NULL);
    prefs_set_tagger_size (p, w, h);

    s = gconf_client_get_string (p->gconf_client, PREF_CARD_FONT, NULL);
    prefs_set_card_font (p, s ? s : DEFAULT_CARD_FONT);
    g_free (s);

    s = gconf_client_get_string (p->gconf_client, PREF_CARD_COLORS, NULL);
    prefs_csvstr_to_colors (p, s ? s : DEFAULT_CARD_COLORS);
    g_free (s);

    s = gconf_client_get_string (p->gconf_client, PREF_SCHEDULES, NULL);
    prefs_csvstr_to_schedules (p, s ? s : DEFAULT_SCHEDULES);
    g_free (s);

    if ((s = gconf_client_get_string (p->gconf_client, PREF_WORKDIR, NULL)))
        prefs_set_workdir (p, s);

    b = gconf_client_get_bool (p->gconf_client, PREF_FIND_WITH_REGEX, NULL);
    prefs_set_find_with_regex (p, b);

    b = gconf_client_get_bool (p->gconf_client, PREF_BACKUP, NULL);
    prefs_set_backup (p, b);

    p->confirm_empty_trash = TRUE;
    b = gconf_client_get_bool (p->gconf_client, PREF_CONFIRM_EMPTY_TRASH, NULL);
    prefs_set_confirm_empty_trash (p, b);

    i = gconf_client_get_int (p->gconf_client, PREF_LATEX_DPI, NULL);
    prefs_set_latex_dpi (p, i);

    return p;
}


void
prefs_free (Prefs *p)
{
    gint i;

    g_free (p->card_font);
    g_free (p->workdir);

    for (i = 0; i < N_CARD_COLORS; i++)
        if (p->color[i])
            gdk_color_free (p->color[i]);

    g_free (p);
}

