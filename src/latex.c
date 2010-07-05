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
#include <glib.h>
#include <glib/gstdio.h>

#include "main.h"
#include "prefs.h"
#include "latex.h"


static GString *gstr;
static GList *texfile;
static gint dpi;
static GRegex *re_math_inline = NULL;
static GRegex *re_latex = NULL;
static gboolean render;
static const gchar *workdir;



#define FNAME_LATEX_PREAMBLE        "latex-preamble"
#define FNAME_LATEX_POSTAMBLE       "latex-postamble"


#define PREAMBLE  "\\documentclass{article}\n"       \
                  "\\pagestyle{empty}\n"             \
                  "\\usepackage[utf8]{inputenc}\n"   \
                  "\\begin{document}\n"
#define POSTAMBLE "\n\\end{document}\n"


static void
remove_temporary_file (const gchar *fname, const gchar *ext)
{
    gchar *s;

    s = g_strdup_printf ("%s%s", fname, ext == NULL ? "" : ext);

#if 0
    g_printerr ("remove_temporary_file: %s\n", s);
#endif

    g_remove (s);

    g_free (s);
}


static void
texfiles_free (void)
{
    GList *cur;

    if (texfile != NULL) {

        for (cur = texfile; cur != NULL; cur = cur->next) {
            remove_temporary_file ((gchar*)cur->data, NULL);
            remove_temporary_file ((gchar*)cur->data, ".aux");
            remove_temporary_file ((gchar*)cur->data, ".dvi");
            remove_temporary_file ((gchar*)cur->data, ".log");
            remove_temporary_file ((gchar*)cur->data, "1.png");
            g_free (cur->data);
        }
        g_list_free (texfile);
        texfile = NULL;

    }
}


void
latex_init (void)
{
    gstr = g_string_new ("");
    texfile = NULL;
    dpi = DEFAULT_LATEX_DPI;

    re_math_inline = g_regex_new ("<\\$>(.+?)</\\$>", G_REGEX_DOTALL, 0, NULL);
    re_latex = g_regex_new ("<latex>(.+?)</latex>", G_REGEX_DOTALL, 0, NULL);
}


static void
latex_set_dpi (gint n)
{
    if (n < 1 || n > MAX_LATEX_DPI)
        return;

    dpi = n;
}


void
latex_free (void)
{
    g_string_free (gstr, TRUE);
    texfiles_free ();

    if (re_math_inline != NULL)
        g_regex_unref (re_math_inline);

    if (re_latex != NULL)
        g_regex_unref (re_latex);
}


static gchar*
latex_render (gboolean math_inline)
{
    gboolean ok;
    gchar *s, *tmp_tex;
    gint fh;


    /* Write PREAMBLE, gstr, and POSTAMBLE to a temporary latex file. */

    tmp_tex = g_build_filename (g_get_tmp_dir (), "ignuit-XXXXXX", NULL);

    ok = ((fh = g_mkstemp (tmp_tex)) != -1);
    if (ok) {

        gchar *preamble = NULL, *postamble = NULL;

        if (!math_inline && workdir != NULL) {

            /* Use non-default preamble and postamble files if present. */

            s = g_build_filename (workdir, FNAME_LATEX_PREAMBLE, NULL);
            g_file_get_contents (s, &preamble, NULL, NULL);
            g_free (s);

            s = g_build_filename (workdir, FNAME_LATEX_POSTAMBLE, NULL);
            g_file_get_contents (s, &postamble, NULL, NULL);
            g_free (s);
        }

        if (preamble == NULL)
            preamble = g_strdup (PREAMBLE);

        if (postamble == NULL)
            postamble = g_strdup (POSTAMBLE);

        s = g_strdup_printf ("%s%s%s", preamble, gstr->str, postamble);
#if 0
        g_printerr ("%s", s);
#endif
        ok = (write (fh, s, strlen (s)) != -1);
        g_free (s);

        close (fh);

        g_free (preamble);
        g_free (postamble);
    }

    if (!ok) {
        g_free (tmp_tex);
        g_warning ("Couldn't make temporary latex file\n");
        return NULL;
    }

    texfile = g_list_append (texfile, tmp_tex);


    /* Run latex */

    if (g_chdir (g_get_tmp_dir ()) != 0) {
        g_warning ("Couldn't change to temporary directory\n");
        return NULL;
    }

    s = g_strdup_printf ("latex -interaction=nonstopmode '%s' 2>&1 1>ignuit-latex.txt", tmp_tex);
    if (system (s) != 0) {
        g_warning ("latex returned non-zero\n");
        g_free (s);
        return NULL;
    }
    g_free (s);


    /* Convert the resulting dvi to a png image. */

    s = g_strdup_printf ("dvipng -q -D %d -bg Transparent -T tight '%s.dvi' 2>&1 1>ignuit-dvipng.txt" , dpi, tmp_tex);
    if (system (s) != 0) {
        g_warning ("dvipng returned non-zero\n");
        g_free (s);
        return NULL;
    }
    g_free (s);

    return g_strdup_printf ("%s1.png", tmp_tex);
}


static gboolean
re_math_inline_eval (GMatchInfo *match_info, GString *result, gpointer data)
{
    const gchar *src;
    gchar *fname_png, *s;


    g_string_truncate (gstr, 0);

    src = g_match_info_fetch (match_info, 1);

    if (render) {
        g_string_append_c (gstr, '$');
        g_string_append (gstr, src);
        g_string_append_c (gstr, '$');
        fname_png = latex_render (TRUE);
        if (fname_png != NULL) {
            s = g_strdup_printf ("<embed type=\"image\" src=\"%s\"/>", fname_png);
            g_free (fname_png);
        }
        else {
            s = g_strdup ("[LaTeX render failed]");
        }
    }
    else {
        s = g_strdup ("[LaTeX]");
    }

    g_string_append (result, s);
    g_free (s);

    return FALSE;
}


static gboolean
re_latex_eval (GMatchInfo *match_info, GString *result, gpointer data)
{
    const gchar *src;
    gchar *fname_png, *s;


    g_string_truncate (gstr, 0);

    src = g_match_info_fetch (match_info, 1);

    if (render) {
        g_string_append (gstr, src);
        fname_png = latex_render (FALSE);
        if (fname_png != NULL) {
            s = g_strdup_printf ("<embed type=\"image\" src=\"%s\"/>", fname_png);
            g_free (fname_png);
        }
        else {
            s = g_strdup ("[LaTeX render failed]");
        }
    }
    else {
        s = g_strdup ("[LaTeX]");
    }

    g_string_append (result, s);
    g_free (s);

    return FALSE;
}


gchar*
latex_preprocess (const gchar *text, const gchar *wdir, gint latex_dpi,
    gboolean do_render)
{
    gchar *s1, *s2;

    texfiles_free ();

    latex_set_dpi (latex_dpi);
    render = do_render;
    workdir = wdir;

    s1 = g_regex_replace_eval (re_math_inline, text, -1, 0, 0,
        (GRegexEvalCallback)re_math_inline_eval, NULL, NULL);

    s2 = g_regex_replace_eval (re_latex, s1, -1, 0, 0,
        (GRegexEvalCallback)re_latex_eval, NULL, NULL);

    g_free (s1);

    return s2;
}

