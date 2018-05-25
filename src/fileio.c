/* ignuit - Educational software for the GNOME, following the Leitner
 * flash-card system.
 *
 * Copyright (C) 2008, 2009, 2015, 2016, 2017 Timothy Richard Musson
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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gnome.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <errno.h>

#include "main.h"
#include "card.h"
#include "file.h"
#include "fileio.h"
#include "csvio.h"
#include "app-window.h"


#define ENABLE_IGNUIT_DEPRECATED 1


typedef enum {
    STATE_START,
    STATE_IN_DECK,
    STATE_IN_CATEGORY,
    STATE_IN_CARD,
    STATE_IN_FRONT,
    STATE_IN_BACK,
    STATE_UNKNOWN
} ParserState;


typedef struct _Parser Parser;


struct _Parser {
    ParserState state;
    ParserState state_last_known;
    gint        depth;
    gint        unknown_depth;
    gboolean    dodgy_file;
    File        *file;
    Card        *card;        /* The currently loading card. */
};


static void
parser_start_unknown (Parser *parser)
{
    if (parser->unknown_depth == 0)
        parser->state_last_known = parser->state;

    parser->state = STATE_UNKNOWN;
    parser->unknown_depth++;
    parser->dodgy_file = TRUE;
}


static void
parser_end_unknown (Parser *parser)
{
    parser->unknown_depth--;

    if (parser->unknown_depth == 0)
        parser->state = parser->state_last_known;

    parser->dodgy_file = TRUE;
}


static const gchar*
attr_value (const gchar *name, const gchar **attrn, const gchar **attrv)
{
    while (*attrn && *attrv) {

        if (strcmp (*attrn, name) == 0)
             return *attrv;

        attrn++;
        attrv++;
    }
    return NULL;
}


static void
start_element_handler (GMarkupParseContext *context,
                       const gchar *element_name,
                       const gchar **attribute_names,
                       const gchar **attribute_values,
                       gpointer user_data, GError **error)
{
    Parser *parser = (Parser*)user_data;
    const gchar *s;

    parser->depth = parser->depth + 1;

    switch (parser->state) {

    case STATE_START:

        if (strcmp (element_name, "deck") == 0) {

            parser->state = STATE_IN_DECK;

            s = attr_value ("title", attribute_names, attribute_values);
            file_set_title (parser->file, s);

            s = attr_value ("author", attribute_names, attribute_values);
            file_set_author (parser->file, s);

#if ENABLE_IGNUIT_DEPRECATED

            /* "comment" and "url" were replaced with "description"
             * and "homepage" in ignuit 0.0.16 */

            s = attr_value ("comment", attribute_names, attribute_values);
            file_set_description (parser->file, s);

            s = attr_value ("url", attribute_names, attribute_values);
            file_set_homepage (parser->file, s);

            s = attr_value ("description", attribute_names, attribute_values);
            if (s != NULL)
                file_set_description (parser->file, s);

            s = attr_value ("homepage", attribute_names, attribute_values);
            if (s != NULL)
                file_set_homepage (parser->file, s);
#else
            s = attr_value ("description", attribute_names, attribute_values);
            file_set_description (parser->file, s);

            s = attr_value ("homepage", attribute_names, attribute_values);
            file_set_homepage (parser->file, s);
#endif
            s = attr_value ("license", attribute_names, attribute_values);
            file_set_license (parser->file, s);

            s = attr_value ("license-uri", attribute_names, attribute_values);
            file_set_license_uri (parser->file, s);

            s = attr_value ("style", attribute_names, attribute_values);
            if (s != NULL)
                file_set_card_style (parser->file, atoi (s));

        }
        else {

            parser_start_unknown (parser);

        }
        break;

    case STATE_IN_DECK:

        if (strcmp (element_name, "category") == 0) {

            Category *cat;

            parser->state = STATE_IN_CATEGORY;

            s = attr_value ("title", attribute_names, attribute_values);
            cat = category_new (s);
            file_add_category (parser->file, cat);
            file_set_current_category (parser->file, cat);

            s = attr_value ("order", attribute_names, attribute_values);
            if (s != NULL) {
                category_set_fixed_order (cat, strcmp (s, "fixed") == 0);
            }

            s = attr_value ("comment", attribute_names, attribute_values);
            if (s != NULL) {
                category_set_comment (cat, s);
            }

        }
        else {

            parser_start_unknown (parser);

        }
        break;

    case STATE_IN_CATEGORY:

        if (strcmp (element_name, "card") == 0) {

            parser->state = STATE_IN_CARD;
            parser->card = card_new ();

            s = attr_value ("grp", attribute_names, attribute_values);
            if (s != NULL)
                card_set_group (parser->card, atoi (s));

            s = attr_value ("crt", attribute_names, attribute_values);
            if (s != NULL)
                card_set_date_created (parser->card, strtoul (s, NULL, 10));

            s = attr_value ("tst", attribute_names, attribute_values);
            if (s != NULL)
                card_set_date_tested (parser->card, strtoul (s, NULL, 10));

            s = attr_value ("exp", attribute_names, attribute_values);
            if (s != NULL)
                card_set_date_expiry (parser->card, strtoul (s, NULL, 10));

            s = attr_value ("time", attribute_names, attribute_values);
            if (s != NULL)
                card_set_time_expiry (parser->card, atoi (s));

            s = attr_value ("n", attribute_names, attribute_values);
            if (s != NULL)
                card_set_n_tests (parser->card, atoi (s));

            s = attr_value ("c", attribute_names, attribute_values);
            if (s != NULL)
                card_set_n_known (parser->card, atoi (s));

#if ENABLE_IGNUIT_DEPRECATED

            /* "t" (for tag) was replaced with "f" (for flag)
             * in ignuit 0.0.16 */

            s = attr_value ("t", attribute_names, attribute_values);
            if (s != NULL)
                card_set_flagged (parser->card, atoi (s) != 0);

#endif
            s = attr_value ("f", attribute_names, attribute_values);
            if (s != NULL)
                card_set_flagged (parser->card, atoi (s) != 0);

            s = attr_value ("tags", attribute_names, attribute_values);
            if (s != NULL)
                file_card_add_new_tags (parser->file,
                    parser->card, s);

        }
        else {

            parser_start_unknown (parser);

        }
        break;

    case STATE_IN_CARD:

        if (strcmp (element_name, "front") == 0)
            parser->state = STATE_IN_FRONT;

        else if (strcmp (element_name, "back") == 0)
            parser->state = STATE_IN_BACK;

        else
            parser_start_unknown (parser);

        break;

    case STATE_IN_FRONT:
    case STATE_IN_BACK:
    case STATE_UNKNOWN:

        parser_start_unknown (parser);

        break;

    }
}


static void
end_element_handler (GMarkupParseContext *context,
                     const gchar *element_name,
                     gpointer user_data,
                     GError **error)
{
    Parser *parser = (Parser*)user_data;

    parser->depth = parser->depth - 1;

    switch (parser->state) {

    case STATE_IN_DECK:
        if (strcmp (element_name, "deck") != 0)
            parser_end_unknown (parser);
        break;

    case STATE_IN_CATEGORY:
        if (strcmp (element_name, "category") == 0)
            parser->state = STATE_IN_DECK;
        else
            parser_end_unknown (parser);
        break;

    case STATE_IN_CARD:
        if (strcmp (element_name, "card") == 0) {
            parser->state = STATE_IN_CATEGORY;
            file_add_loaded_card (parser->file, parser->card);
            parser->card = NULL;
        }
        else {
            parser_end_unknown (parser);
        }
        break;

    case STATE_IN_FRONT:
        if (strcmp (element_name, "front") == 0)
            parser->state = STATE_IN_CARD;
        else
            parser_end_unknown (parser);
        break;

    case STATE_IN_BACK:
        if (strcmp (element_name, "back") == 0)
            parser->state = STATE_IN_CARD;
        else
            parser_end_unknown (parser);
        break;

    case STATE_UNKNOWN:
        parser_end_unknown (parser);
        break;

    case STATE_START:
        break;

    }
}


static void
text_handler (GMarkupParseContext *context,
              const gchar *text,
              gsize text_len,
              gpointer user_data,
              GError **error)
{
    Parser *parser = (Parser*)user_data;

    switch (parser->state) {
    case STATE_IN_FRONT:
        card_set_front (parser->card, text);
        break;
    case STATE_IN_BACK:
        card_set_back (parser->card, text);
        break;
    default:
        break;
    }
}


static GMarkupParser handlers = {
    start_element_handler,
    end_element_handler,
    text_handler,
    NULL,
    NULL
};


File*
fileio_load (const gchar *fname, GError **err)
{
    GMarkupParseContext *context;
    gchar    *contents = NULL;
    gsize    len;
    gboolean ok;
    Parser   parser;


    /* Load a native ignuit XML file. */

    if (!g_file_get_contents (fname, &contents, &len, err))
        return NULL;

    if (!g_utf8_validate (contents, -1, NULL)) {
        g_set_error (err, 0, 0, _("The file is not valid UTF-8."));
        return NULL;
    }

    parser.state = STATE_START;
    parser.depth = 0;
    parser.unknown_depth = 0;
    parser.dodgy_file = FALSE;
    parser.card = NULL;
    parser.file = file_new ();

    file_set_filename (parser.file, fname);

    context = g_markup_parse_context_new (&handlers, 0, &parser, NULL);
    ok = g_markup_parse_context_parse (context, contents, len, NULL);

    g_markup_parse_context_free (context);
    g_free (contents);

    if (ok && parser.dodgy_file) {
        if (file_get_n_cards (parser.file) == 0) {
            g_set_error (err, 0, 0,
                _("Please check that this is an ignuit flashcard file."));
        }
        else {
            g_set_error (err, 0, 0,
                _("The file contains unknown elements."));
        }
    }

    if (ok) {
        file_set_current_category (parser.file, NULL);
        file_check_expired (parser.file);
        return parser.file;
    }

    /* Don't use the GError set by g_markup_parse_context_parse, because
     * it likely contains markup that we don't want to feed into an error
     * dialog. */

    g_set_error (err, 0, 0, _("Error while parsing '%s'"), fname);

    if (parser.card != NULL)
        card_free (parser.card);

    file_free (parser.file, TRUE);

    return NULL;
}


static void
write_card_xml (FILE *fp, Card *c)
{
    const gchar *text;
    gchar       *s;
    guint32     date;
    gint        n, hour;


    fprintf (fp, "<card grp='%d'", card_get_group (c));

    if ((date = card_get_date_created (c)) != G_DATE_BAD_JULIAN)
        fprintf (fp, " crt='%d'", date);

    if ((date = card_get_date_tested (c)) != G_DATE_BAD_JULIAN)
        fprintf (fp, " tst='%d'", date);

    if ((date = card_get_date_expiry (c)) != G_DATE_BAD_JULIAN) {
        fprintf (fp, " exp='%d'", date);
        if ((hour = card_get_time_expiry (c)) != 0)
            fprintf (fp, " time='%d'", hour);
    }

    n = card_get_n_tests (c);
    if (n > 0)
        fprintf (fp, " n='%d'", n);

    n = card_get_n_known (c);
    if (n > 0)
        fprintf (fp, " c='%d'", n);

    if (card_get_flagged (c))
        fprintf (fp, " f='1'");

    if (card_get_tags (c) != NULL) {

        gchar *tags;

        tags = card_get_tags_as_string (c);
        s = g_markup_printf_escaped ("%s", tags);
        fprintf (fp, " tags='%s'", s);
        g_free (s);
        g_free (tags);
    }

    text = card_get_front (c);
    s = g_markup_printf_escaped ("><front>%s</front>", text);
    fprintf (fp, "%s", s);
    g_free (s);

    text = card_get_back (c);
    s = g_markup_printf_escaped ("<back>%s</back></card>\n", text);
    fprintf (fp, "%s", s);
    g_free (s);
}


static void
write_category_xml (FILE *fp, Category *cat)
{
    GList *cur;
    gchar *s;


    s = g_markup_escape_text (category_get_title (cat), -1);
    fprintf (fp, "<category title='%s'", s);
    g_free (s);

    if (category_is_fixed_order (cat))
        fprintf (fp, " order='fixed'");

    s = g_markup_escape_text (category_get_comment (cat), -1);
    fprintf (fp, " comment='%s'", s);
    g_free (s);

    fprintf (fp, ">\n");

    for (cur = category_get_cards (cat); cur != NULL; cur = cur->next)
        write_card_xml (fp, CARD(cur));

    fprintf (fp, "</category>\n");
}


static void
write_file_xml (Ignuit *ig, FILE *fp)
{
    GList *cur;
    gchar *s;


    file_set_category_order (ig->file, app_window_get_category_list (ig));

    fprintf (fp, "<?xml version='1.0' encoding='UTF-8'?>\n");

    fprintf (fp, "<deck version='1'");

    s = g_markup_escape_text (file_get_title (ig->file), -1);
    fprintf (fp, " title='%s'", s);
    g_free (s);

    s = g_markup_escape_text (file_get_author (ig->file), -1);
    fprintf (fp, " author='%s'", s);
    g_free (s);

    s = g_markup_escape_text (file_get_description (ig->file), -1);
    fprintf (fp, " description='%s'", s);
    g_free (s);

    s = g_markup_escape_text (file_get_homepage (ig->file), -1);
    fprintf (fp, " homepage='%s'", s);
    g_free (s);

    s = g_markup_escape_text (file_get_license (ig->file), -1);
    fprintf (fp, " license='%s'", s);
    g_free (s);

    s = g_markup_escape_text (file_get_license_uri (ig->file), -1);
    fprintf (fp, " license-uri='%s'", s);
    g_free (s);

    fprintf (fp, " style='%d'", file_get_card_style (ig->file));

    fprintf (fp, ">\n");

    for (cur = file_get_category_order (ig->file); cur != NULL; cur = cur->next)
        write_category_xml (fp, CATEGORY(cur));

    fprintf (fp, "</deck>\n");
}


static gboolean
make_backup (const gchar *fname, GError **err)
{
    gchar *fname_backup;
    gboolean ok;

    if (!g_file_test (fname, G_FILE_TEST_EXISTS))
        return TRUE;

    if (!g_file_test (fname, G_FILE_TEST_IS_REGULAR)) {
        g_set_error (err, 0, 0, "Not a regular file: '%s'", fname);
        return FALSE;
    }

    fname_backup = g_strdup_printf ("%s~", fname);

    ok = (g_rename (fname, fname_backup) == 0);
    if (!ok) {
        g_set_error (err, 0, 0, "Failed to rename '%s' as '%s'",
            fname, fname_backup);
    }

    g_free (fname_backup);

    return ok;
}


gboolean
fileio_save (Ignuit *ig, const gchar *fname, GError **err)
{
    FILE *f_strm;


    /* Write a native ignuit XML file. */

    if (prefs_get_backup (ig->prefs) && !make_backup (fname, err))
        return FALSE;

    f_strm = fopen (fname, "w");
    if (!f_strm) {
        g_set_error (err, 0, 0, "%s", g_strerror (errno));
        return FALSE;
    }

    write_file_xml (ig, f_strm);

    fclose (f_strm);

    file_set_changed (ig->file, FALSE);

    return TRUE;
}


static void
load_card_from_csv_row (File *f, GList *row)
{
    GList *item;
    Card *c;

    c = card_new ();

    if ((item = g_list_nth (row, 0)) != NULL)
        card_set_front (c, (gchar*)item->data);

    if ((item = g_list_nth (row, 1)) != NULL)
        card_set_back (c, (gchar*)item->data);

    if ((item = g_list_nth (row, 2)) != NULL) {

        Category *cat;
        gchar *title;

        title = (gchar*)item->data;
        cat = file_lookup_category (f, title);

        if (cat == NULL) {
            cat = category_new (title);
            file_add_category (f, cat);
        }
        file_set_current_category (f, cat);  
    }

    if (file_get_current_category (f) == NULL) {
        Category *cat = category_new (NULL);
        file_add_category (f, cat);
        file_set_current_category (f, cat);
    }

    file_add_loaded_card (f, c);
}


File*
fileio_import_csv (const gchar *fname, gchar delimiter, GError **err)
{
    GError   *parse_err = NULL;
    Csv      *csv;
    File     *f;
    GList    *row;
    gchar    *contents;


    /* Import a file from Comma or Tab Separated Values. */

    /* First check for valid UTF-8. */

    if (!g_file_get_contents (fname, &contents, NULL, err))
        return NULL;

    if (!g_utf8_validate (contents, -1, NULL)) {
        g_set_error (err, 0, 0, _("The file is not valid UTF-8."));
        g_free (contents);
        return NULL;
    }
    g_free (contents);


    /* Now parse the file. */

    if ((csv = csv_open_r (fname, delimiter, err)) == NULL)
        return NULL;

    f = file_new ();

    while ((row = csv_read_row (csv, &parse_err)) != NULL)
        load_card_from_csv_row (f, row);

    csv_close (csv);

    if (parse_err != NULL) {
        g_set_error (err, 0, 0, _("Error while parsing file."));
        file_free (f, TRUE);
        f = NULL;
        g_error_free (parse_err);
    }
    else if (file_get_n_cards (f) == 0) {
        g_set_error (err, 0, 0, _("No cards were found in the imported file."));
        file_free (f, TRUE);
        f = NULL;
    }

    return f;
}


File*
fileio_import_xml (Ignuit *ig, const gchar *fname, const gchar *filter,
    GError **err)
{
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;
    gchar     *fname_xsl_full;
    gchar     *fname_xsl;
    gchar     *fname_tmp;
    FILE      *f_strm;
    gint      f_des;
    File      *f;


    /* Import a non-native XML file using libxslt. */

    fname_xsl = g_build_filename (IMPORT_DIR, filter, NULL);

    fname_xsl_full = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, fname_xsl, TRUE, NULL);

    g_free (fname_xsl);

    if (fname_xsl_full == NULL) {
        g_set_error (err, 0, 0, "Can't find import filter '%s'", filter);
        return NULL;
    }

    f_des = g_file_open_tmp ("ignuitXXXXXX", &fname_tmp, err);
    if (f_des == -1) {
        g_free (fname_tmp);
        return NULL;
    }
    f_strm = fdopen (f_des, "w");

    xmlSubstituteEntitiesDefault (0);
    xmlLoadExtDtdDefaultValue = 0;

    cur = xsltParseStylesheetFile ((const xmlChar*)fname_xsl_full);
    doc = xmlParseFile (fname);
    res = xsltApplyStylesheet (cur, doc, NULL);
    xsltSaveResultToFile (f_strm, res, cur);

    xsltFreeStylesheet (cur);
    xmlFreeDoc (res);
    xmlFreeDoc (doc);

    xsltCleanupGlobals ();
    xmlCleanupParser ();

    g_free (fname_xsl_full);

    fclose (f_strm);
    close (f_des);

    f = fileio_load (fname_tmp, NULL);
    if (f == NULL) {
        g_set_error (err, 0, 0,
            _("Please check that imported file is not compressed, and that the correct filter is selected."));
    }
    else if (file_get_n_cards (f) == 0) {
        g_clear_error (err);
        g_set_error (err, 0, 0, _("No cards were found in the imported file."));
        file_free (f, TRUE);
        f = NULL;
    }

    g_remove (fname_tmp);
    g_free (fname_tmp);

    return f;
}


gboolean
fileio_export_csv (File *f, const gchar *fname, gchar delimiter,
    gboolean excl_markup, GError **err)
{
    GList *cur;
    Csv *csv;
    Card *c;


    /* Export a file as Comma or Tab Separated Values. */

    if ((csv = csv_open_w (fname, delimiter, err)) == NULL)
        return FALSE;

    for (cur = file_get_cards (f); cur != NULL; cur = cur->next) {

        c = CARD(cur);

        csv_row_clear (csv);
        if (excl_markup) {
            csv_row_add_field (csv, card_get_front_without_markup (c));
            csv_row_add_field (csv, card_get_back_without_markup (c));
        }
        else {
            csv_row_add_field (csv, card_get_front (c));
            csv_row_add_field (csv, card_get_back (c));
        }
        csv_row_add_field (csv, category_get_title (card_get_category (c)));

        csv_write_row (csv);

    }

    csv_close (csv);

    return TRUE;
}


gboolean
fileio_export_xml (Ignuit *ig, const gchar *fname, const gchar *filter,
    gboolean excl_markup, GError **err)
{
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;
    gchar     *fname_xsl;
    gchar     *fname_xsl_full;
    gchar     *fname_tmp;
    FILE      *f_strm_out, *f_strm_tmp;
    gint      f_des_tmp;


    /* Use libxslt to write a file in a non-native XML format. */

    fname_xsl = g_build_filename (EXPORT_DIR, filter, NULL);

    fname_xsl_full = gnome_program_locate_file (ig->program,
        GNOME_FILE_DOMAIN_APP_DATADIR, fname_xsl, TRUE, NULL);

    g_free (fname_xsl);

    if (fname_xsl_full == NULL) {
        g_set_error (err, 0, 0, "Can't find export filter '%s'", filter);
        return FALSE;
    }

    f_des_tmp = g_file_open_tmp ("ignuitXXXXXX", &fname_tmp, err);
    if (f_des_tmp == -1) {
        g_free (fname_tmp);
        return FALSE;
    }
    f_strm_tmp = fdopen (f_des_tmp, "w");

    write_file_xml (ig, f_strm_tmp);

    fclose (f_strm_tmp);
    close (f_des_tmp);

    f_strm_out = fopen (fname, "w");
    if (!f_strm_out) {
        g_set_error (err, 0, 0, "%s", g_strerror (errno));
        return FALSE;
    }

    xmlSubstituteEntitiesDefault (0);
    xmlLoadExtDtdDefaultValue = 0;

    cur = xsltParseStylesheetFile ((const xmlChar*)fname_xsl_full);
    doc = xmlParseFile (fname_tmp);
    res = xsltApplyStylesheet (cur, doc, NULL);
    xsltSaveResultToFile (f_strm_out, res, cur);

    xsltFreeStylesheet (cur);
    xmlFreeDoc (res);
    xmlFreeDoc (doc);

    xsltCleanupGlobals ();
    xmlCleanupParser ();

    g_free (fname_xsl_full);

    fclose (f_strm_out);

    g_remove (fname_tmp);
    g_free (fname_tmp);

    return TRUE;
}

