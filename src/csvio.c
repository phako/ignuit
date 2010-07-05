/* csvio.c - CSV/TSV file input/output
 *
 * Copyright (C) 2009 Timothy Richard Musson
 *
 * Email: Tim Musson <trmusson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*

gcc -Wall -DTEST csvio.c `pkg-config --cflags --libs glib-2.0` -o csviotest

See:

  http://en.wikipedia.org/wiki/Comma-separated_values
  http://www.creativyst.com/Doc/Articles/CSV/CSV01.htm

An example CSV file from Wikipedia:

1997,Ford,E350,"ac, abs, moon",3000.00
1999,Chevy,"Venture ""Extended Edition""","",4900.00
1996,Jeep,Grand Cherokee,"MUST SELL!
air, moon roof, loaded",4799.00

*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* strerror */
#include <errno.h>
#include <glib.h>

#include "csvio.h"


typedef enum {
    CSV_TOKEN_NONE = 0,
    CSV_TOKEN_DATA,
    CSV_TOKEN_NEW_LINE,
    CSV_TOKEN_DELIMITER,
    CSV_TOKEN_BEGIN_QUOTE,
    CSV_TOKEN_ERROR
} CsvToken;


struct _Csv {

    gchar       *fname;
    FILE        *fp;
    GList       *row;
    GString     *data;
    CsvToken    token;
    CsvToken    prev_token;
    gboolean    quoted;
    gchar       delimiter;

};


Csv*
csv_open_r (const gchar *fname, gchar delimiter, GError **err)
{
    FILE *fp;
    Csv *csv;

    if ((fp = fopen (fname, "r")) == NULL) {
        if (err != NULL)
            g_set_error (err, 0, 0, "%s", strerror (errno));
        return NULL;
    }

    csv = g_new0 (Csv, 1);

    csv->fp = fp;
    csv->fname = g_strdup (fname);
    csv->delimiter = delimiter;
    csv->data = g_string_new ("");

    return csv;
}


Csv*
csv_open_w (const gchar *fname, gchar delimiter, GError **err)
{
    FILE *fp;
    Csv *csv;

    if ((fp = fopen (fname, "w")) == NULL) {
        if (err != NULL)
            g_set_error (err, 0, 0, "%s", strerror (errno));
        return NULL;
    }

    csv = g_new0 (Csv, 1);

    csv->fp = fp;
    csv->fname = g_strdup (fname);
    csv->delimiter = delimiter;

    return csv;
}


void
csv_close (Csv *csv)
{
    g_free (csv->fname);

    csv_row_clear (csv);

    if (csv->data != NULL)
        g_string_free (csv->data, TRUE);

    if (csv->fp != NULL)
        fclose (csv->fp);

    g_free (csv);
}


const gchar*
csv_get_filename (Csv *csv)
{
    return csv->fname;
}


static gchar*
csv_escape_text (const gchar *text)
{
    GString *gs;
    const gchar *s;
    gchar *ret;

    /* Quote characters are escaped by doubling them up. */

    gs = g_string_new ("");
    for (s = text; *s != '\0'; s++) {
        if (*s == '"')
            g_string_append_c (gs, '"');
        g_string_append_c (gs, *s);
    }

    ret = gs->str;

    g_string_free (gs, FALSE);

    return ret;
}


void
csv_row_clear (Csv *csv)
{
    if (csv->row != NULL) {
        g_list_foreach (csv->row, (GFunc)g_free, NULL);
        g_list_free (csv->row);
        csv->row = NULL;
    }
}


void
csv_row_add_field (Csv *csv, const gchar *text)
{
    csv->row = g_list_append (csv->row, csv_escape_text (text));
}


void
csv_write_row (Csv *csv)
{
    GList *cur;

    for (cur = csv->row; cur != NULL; cur = cur->next) {
        fprintf (csv->fp, "\"%s\"", (gchar*)cur->data);
        if (cur->next != NULL)
            fprintf (csv->fp, "%c", csv->delimiter);
    }
    fprintf (csv->fp, "\n");
}


static gboolean
csv_find_data (Csv *csv)
{
    gint c;

    if (csv->delimiter != '\t')
        while (g_ascii_isspace (c = fgetc (csv->fp)) && c != '\n');
    else
        while ((c = fgetc (csv->fp)) == ' ');

    ungetc (c, csv->fp);

    return c != EOF;
}


static CsvToken
csv_get_token (Csv *csv)
{
    gboolean done;
    gint c;

    if (csv->prev_token != CSV_TOKEN_BEGIN_QUOTE)
        if (!csv_find_data (csv))
            return CSV_TOKEN_NONE;

    c = getc (csv->fp);

    if (csv->prev_token == CSV_TOKEN_NEW_LINE && c == '\n')
        return CSV_TOKEN_NEW_LINE;

    if (csv->prev_token == CSV_TOKEN_DATA) {

        if (c == csv->delimiter)
            return CSV_TOKEN_DELIMITER;
        else if (c == '\n')
            return CSV_TOKEN_NEW_LINE;

        return CSV_TOKEN_ERROR;
    }

    if (c == '"' && !csv->quoted) {
        csv->quoted = TRUE;
        return CSV_TOKEN_BEGIN_QUOTE;
    }

    ungetc (c, csv->fp);
    g_string_assign (csv->data, "");

    while (1) {

        c = getc (csv->fp);

        done = (c == EOF)
            || (c == csv->delimiter && !csv->quoted)
            || (c == '\n' && !csv->quoted);

        if (done) {
            ungetc (c, csv->fp);
            return CSV_TOKEN_DATA;
        }

        if (c == '"') {
            gint next = getc (csv->fp);
            if (next != '"') {
                ungetc (next, csv->fp);
                csv->quoted = FALSE;
                return CSV_TOKEN_DATA;
            }
        }
        g_string_append_c (csv->data, c);
    }
}


GList*
csv_read_row (Csv *csv, GError **err)
{
    csv_row_clear (csv);

    csv->prev_token = CSV_TOKEN_NONE;

    while (1) {

        csv->token = csv_get_token (csv);

        switch (csv->token) {
        case CSV_TOKEN_DATA:
            csv->row = g_list_append (csv->row,
                g_strdup ((gchar*)csv->data->str));
            break;
        case CSV_TOKEN_NEW_LINE:
        case CSV_TOKEN_NONE:
            return csv->row;
            break;
        case CSV_TOKEN_ERROR:
            g_set_error (err, 0, 0, "Error while parsing file");
            return NULL;
            break;
        default:
            break;
        }

        csv->prev_token = csv->token;
    }
}


#ifdef TEST

#define DELIMITER ','  /* ',' for CSV, '\t' for TSV */


/*

A CSV writing example would look something like this:

csv = csv_open_w (fname, delimiter, NULL);

for (i = 0; i < 10; i++) {
    csv_row_clear (csv);
    csv_row_add_field (csv, "foo");
    csv_row_add_field (csv, "bar");
    csv_row_add_field (csv, "baz");
    csv_write_row (csv);
}

csv_close (csv);

*/

int
main (int argc, char *argv[])
{
    GError *err = NULL;
    Csv *csv;
    GList *cur;


    if (argc != 2) {
        g_printerr ("Usage: %s CSVFILE\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    if ((csv = csv_open_r (argv[1], DELIMITER, &err)) == NULL) {
        g_printerr ("%s: %s\n", argv[0], err->message);
        g_error_free (err);
        exit (EXIT_FAILURE);
    }

    while (csv_read_row (csv, &err) != NULL) {
        for (cur = csv->row; cur != NULL; cur = cur->next)
            g_print ("{%s}\n", (gchar*)cur->data);
        g_print ("\n");
    }

    if (err) {
        g_printerr ("%s\n", err->message);
        g_error_free (err);
    }

    csv_close (csv);

    return EXIT_SUCCESS;
}


#endif

