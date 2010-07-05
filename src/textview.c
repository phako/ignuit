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
#include <glib.h>
#include <glib/gi18n.h>
#include <gnome.h>

#include "main.h"
#include "card.h"
#include "prefs.h"
#include "textview.h"
#include "audio.h"
#include "latex.h"


typedef enum {
    TAG_OPEN = 0,
    TAG_CLOSE,
    TAG_SELF_CLOSE
} MTagType;


typedef enum {
    TOKEN_NONE = 0,
    TOKEN_TEXT,
    TOKEN_NEW_LINE,
    TOKEN_TAG
} Token;


typedef enum {

    ELEMENT_ID_UNKNOWN = 0,
    ELEMENT_ID_SPAN,
    ELEMENT_ID_KEYWORD,
    ELEMENT_ID_ITALIC,
    ELEMENT_ID_UNDERLINE,
    ELEMENT_ID_BIG,
    ELEMENT_ID_BOLD,
    ELEMENT_ID_SMALL,
    ELEMENT_ID_SUB,
    ELEMENT_ID_SUP,
    ELEMENT_ID_STRIKETHROUGH,
    ELEMENT_ID_TT,
    ELEMENT_ID_CENTER,
    ELEMENT_ID_LEFT,
    ELEMENT_ID_RIGHT,
    ELEMENT_ID_VERBATIM,

    /* Add new LaTeX tags only after ELEMENT_ID_MATH_INLINE. */

    ELEMENT_ID_MATH_INLINE,
    ELEMENT_ID_LATEX,

    /* Add new self-closing tags only after ELEMENT_ID_BR. */

    ELEMENT_ID_BR,
    ELEMENT_ID_EMBED

} ElementID;


typedef struct _Element Element;


struct _Element {
    gchar     *name;
    gint      length;
};


static Element element[] = {
    { NULL,       0 }, /* ELEMENT_ID_UNKNOWN */
    { "span",     4 },
    { "key",      3 },
    { "i",        1 },
    { "u",        1 },
    { "big",      3 },
    { "b",        1 },
    { "small",    5 },
    { "sub",      3 },
    { "sup",      3 },
    { "s",        1 },
    { "tt",       2 },
    { "center",   6 },
    { "left",     4 },
    { "right",    5 },
    { "verbatim", 8 },
    { "$",        1 },
    { "latex",    5 },
    { "br",       2 },
    { "embed",    5 }
};


static gint n_elements = sizeof (element) / sizeof (element[0]);


typedef struct _Attr Attr;

struct _Attr {
    gchar *name;
    gchar *value;
};


typedef struct _Parser Parser;

struct _Parser {

    GString   *data;
    Token     token;
    gint      length;
    ElementID element_id;
    MTagType  tag_type; /* open, close, or self-closing */
    gboolean  in_keyword;
    gboolean  verbatim;
    gboolean  error;
    GList     *tags;
    GList     *attr;
    gint      side;

};


typedef struct _MTag MTag;

struct _MTag {

    ElementID element_id;
    GList     *attr;
    gint      offset;

};


static const gchar *workdir = NULL;


GtkJustification
card_style_get_justification (CardStyle style)
{
    if (style == CARD_STYLE_NONE
        || style == CARD_STYLE_KEYWORDS_AND_CENTERED_TEXT) {
        return GTK_JUSTIFY_CENTER;
    }
    return GTK_JUSTIFY_LEFT;
}


void
textbuf_create_tags (GtkTextBuffer *textbuf)
{
    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_SPAN].name,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_ITALIC].name,
        "style", PANGO_STYLE_ITALIC,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_BOLD].name,
        "weight", PANGO_WEIGHT_BOLD,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_UNDERLINE].name,
        "underline", PANGO_UNDERLINE_SINGLE,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_STRIKETHROUGH].name,
        "strikethrough", TRUE,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_BIG].name,
        "scale", PANGO_SCALE_X_LARGE,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_SMALL].name,
        "scale", PANGO_SCALE_X_SMALL,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_SUP].name,
        "rise", 10 * PANGO_SCALE,
        "size", 7 * PANGO_SCALE,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_SUB].name,
        "rise", -10 * PANGO_SCALE,
        "size", 7 * PANGO_SCALE,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_TT].name,
        "family", "monospace",
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_KEYWORD].name,
        "scale", PANGO_SCALE_XX_LARGE,
        "pixels-above-lines", 6,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_CENTER].name,
        "justification", GTK_JUSTIFY_CENTER,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_LEFT].name,
        "justification", GTK_JUSTIFY_LEFT,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_RIGHT].name,
        "justification", GTK_JUSTIFY_RIGHT,
        NULL);

    gtk_text_buffer_create_tag (textbuf,
        element[ELEMENT_ID_VERBATIM].name,
        NULL);
}


void
textview_set_properties (GtkTextView *textview, Prefs *prefs,
    CardStyle style, TVType tvtype)
{
    GdkColor *color;
    const gchar *font;


    color = prefs_get_color (prefs, COLOR_CARD_FG);
    gtk_widget_modify_text (GTK_WIDGET(textview), GTK_STATE_NORMAL, color);

    switch (tvtype) {
    case TV_EDITOR:
    case TV_QUIZ:
        color = prefs_get_color (prefs, COLOR_CARD_BG);
        break;
    case TV_INFO:
        color = prefs_get_color (prefs, COLOR_CARD_BG_END);
        break;
    }
    gtk_widget_modify_base (GTK_WIDGET(textview), GTK_STATE_NORMAL, color);

    if ((font = prefs_get_card_font (prefs)) != NULL) {

        PangoFontDescription *desc;

        desc = pango_font_description_from_string (font);
        gtk_widget_modify_font (GTK_WIDGET(textview), desc);
        pango_font_description_free (desc);

    }

    gtk_text_view_set_left_margin (textview, 6);
    gtk_text_view_set_right_margin (textview, 6);

    gtk_text_view_set_editable (textview, tvtype == TV_EDITOR);
    gtk_text_view_set_cursor_visible (textview, tvtype == TV_EDITOR);

    gtk_text_view_set_justification (GTK_TEXT_VIEW(textview),
        card_style_get_justification (style));
}


void
textbuf_clear (GtkTextBuffer *textbuf)
{
    GtkTextIter start, end;

    gtk_text_buffer_get_start_iter (textbuf, &start);
    gtk_text_buffer_get_end_iter (textbuf, &end);
    gtk_text_buffer_delete (textbuf, &start, &end);
}


void
textbuf_put (GtkTextBuffer *textbuf, const gchar *text)
{
    GtkTextIter iter;

    gtk_text_buffer_get_end_iter (textbuf, &iter);
    gtk_text_buffer_insert (textbuf, &iter, text, -1);
}


void
textbuf_place_cursor (GtkTextBuffer *textbuf, GtkTextIter *iter, gint where)
{
    gtk_text_buffer_get_iter_at_offset (textbuf, iter, where);
    gtk_text_buffer_place_cursor (textbuf, iter);
}


static void
attr_free (Attr *attr)
{
    if (attr == NULL) { return; }

#if 0
    g_printerr ("attr_free: %s = %s\n", attr->name, attr->value);
#endif
    g_free (attr->name);
    g_free (attr->value);
    g_free (attr);
}


static void
attr_list_free (GList *attr)
{
    if (attr != NULL) {

        GList *cur;

        for (cur = attr; cur != NULL; cur = cur->next) {
            attr_free ((Attr*)cur->data);
        }

        g_list_free (attr);

    }
}


static const gchar*
attr_list_get_value (GList *attr, const gchar *name)
{
    GList *cur;
    Attr *a;

    for (cur = attr; cur != NULL; cur = cur->next) {

        a = (Attr*)cur->data;

        if (strcmp (a->name, name) == 0) {
            return a->value;
        }

    }

    return NULL;
}


static void
mtag_free (MTag *t)
{
    if (t->attr != NULL) {

        GList *cur;

        for (cur = t->attr; cur != NULL; cur = cur->next) {
            attr_free ((Attr*)cur->data);
        }

        g_list_free (t->attr);

    }

    g_free (t);
}


static Parser*
parser_new (void)
{
    Parser *p;

    p = g_new0 (Parser, 1);
    p->data = g_string_new ("");

    return p;
}


static void
parser_free (Parser *p)
{
    g_string_free (p->data, TRUE);

    if (p->tags != NULL) {

        GList *cur;

        for (cur = p->tags; cur != NULL; cur = cur->next) {
            mtag_free ((MTag*)cur->data);
        }
        g_list_free (p->tags);

    }

    attr_list_free (p->attr);
    g_free (p);
}


static gint
skip_space (const gchar *str)
{
    gint length;

    for (length = 0; g_ascii_isspace (*str); length++, str++);
    return length;
}


static Attr*
parse_attr (const gchar *str, gint *len)
{
    Attr *attr;
    const gchar *p, *start_name, *start_value;
    gchar quote;
    gint i;

    *len = skip_space (str);
    start_name = p = str + *len;

    i = 0;
    while (g_ascii_isalpha (*p)) {
        i++;
        p++;
    }

    if (i == 0) { return NULL; }

    *len = *len + i;

    if (*p != '=') {
        /* Attribute name not followed by '='. */
        *len = -1;
        return NULL;
    }

    p++;
    *len = *len + 1;

    if (*p != '\'' && *p != '"') {
        /* Attribute value not quoted. */
        g_warning ("attribute value not quoted\n");
        *len = -1;
        return NULL;
    }
    quote = *p;

    p++;
    *len = *len + 1;


    /* Store the attribute name and value. */

    attr = g_new0 (Attr, 1);
    attr->name = g_strndup (start_name, i);

    /* XXX: What if the value contains a quote character? */
    start_value = p;
    i = 0;
    while (*p != '\0' && *p != quote) {
        i++;
        p++;
    }

    if (*p != quote) {
        /* Missing end quote. */
        attr_free (attr);
        *len = -1;
        return NULL;
    }
    *len = *len + i + 1;

    attr->value = g_strndup (start_value, i);

    return attr;
}


static GList*
parse_tag (const gchar *str, MTagType *tag_type, gint *length, ElementID *id)
{
    GList *attr;
    Attr *a;
    gint attr_length;
    const gchar *p;


    *id = ELEMENT_ID_UNKNOWN;

    str++;
    if (*str == '/') {
        *tag_type = TAG_CLOSE;
        str++;
    }
    else {
        *tag_type = TAG_OPEN;
    }

    if (*str == '\0') {
        return NULL;
    }

    attr = NULL;

    for (*id = 1; *id < n_elements; *id = *id + 1) {

        *length = element[*id].length;
        p = str + *length;

        if (strncmp (str, element[*id].name, *length) == 0
            && !g_ascii_isalpha (*p)) {

            gint pos;

            pos = *length;

            while ((a = parse_attr (str + pos, &attr_length)) != NULL
                && attr_length != -1) {
                attr = g_list_append (attr, a);
                pos = pos + attr_length;
                *length = *length + attr_length;
            }
            if (attr_length == -1) {
                g_warning ("parse_tag: bad attribute for <%s>", element[*id].name);
                attr_free (a);
                attr_list_free (attr);
                *id = ELEMENT_ID_UNKNOWN;
                return NULL;
            }
            if (a == NULL) {
                /* whitespace */
                *length = *length + attr_length;
            }

            if (*tag_type == TAG_OPEN && str[*length] == '/') {
                *tag_type = TAG_SELF_CLOSE;
                *length = *length + 1;
            }

            if (str[*length] == '>') {

                switch (*tag_type) {
                case TAG_OPEN:
                    *length = *length + 2;
                    break;
                case TAG_CLOSE:
                    *length = *length + 3;
                    break;
                case TAG_SELF_CLOSE:
                    *length = *length + 2;
                    break;
                }

                return attr;

            }
        }

    }

    *id = ELEMENT_ID_UNKNOWN;

    return NULL;
}


static Token
parser_get_token (Parser *p, const gchar *text)
{
    const gchar *s, *s_start;
    ElementID id;
    gint length;


    p->attr = NULL;

    s = s_start = text;
    g_string_set_size (p->data, 0);

    p->length = 0;

    if (*s == '\0') {
        if (p->in_keyword) {
            p->token = TOKEN_NEW_LINE;
        }
        else {
            p->token = TOKEN_NONE;
        }
        return p->token;
    }

    if (*s == '<' && !p->error) {

        p->attr = parse_tag (s, &p->tag_type, &length, &id);
        p->length = length;
        p->element_id = id;

        if (id == ELEMENT_ID_VERBATIM && p->tag_type == TAG_CLOSE) {
            p->verbatim = FALSE;
            p->token = TOKEN_TAG;
        }
        else if (id != ELEMENT_ID_UNKNOWN && !p->verbatim) {
            if (id == ELEMENT_ID_VERBATIM) {
                p->verbatim = TRUE;
            }
            p->token = TOKEN_TAG;
        }
        else {
            /* Treat this as plain text. */
            p->token = TOKEN_TEXT;
            g_string_overwrite_len (p->data, 0, s, p->length);
        }

        return p->token;
    }


    if (*s == '\n') {

        p->length = 1;
        p->token = TOKEN_NEW_LINE;

        g_string_assign (p->data, "\n");

        return p->token;
    }

    while (*s != '\0') {

        if (*s == '<' && s != s_start) {
            break;
        }
        if (*s == '\n') {
            break;
        }

        s++;

        p->length = p->length + 1;
    }

    g_string_overwrite_len (p->data, 0, s_start, p->length);

    p->token = TOKEN_TEXT;

    return p->token;
}


static void
display_open_tag (GtkTextBuffer *textbuf, Parser *p)
{
    GtkTextIter start;
    MTag *tag;

    gtk_text_buffer_get_end_iter (textbuf, &start);

    tag = g_new0 (MTag, 1);
    tag->element_id = p->element_id;
    tag->offset = gtk_text_iter_get_offset (&start);
    tag->attr = p->attr;

    p->tags = g_list_prepend (p->tags, tag);
    p->attr = NULL;
}


static void
display_close_tag (GtkTextView *textview, GtkTextBuffer *textbuf, Parser *p)
{
    GtkTextIter start, end;
    MTag *tag;


    if (p->tags == NULL) {
        /* Trying to close an element before ever opening one. */
        g_warning ("display_close_tag: open/close mismatch");
        p->error = TRUE;
        return;
    }

    if ((tag = g_list_first (p->tags)->data) != NULL) {

        if (tag->element_id != p->element_id) {
            /* This close tag doesn't match the last open tag. */
            g_warning ("display_close_tag: open/close mismatch");
            p->error = TRUE;
            return;
        }

        gtk_text_buffer_get_iter_at_offset (textbuf, &start, tag->offset);
        gtk_text_buffer_get_end_iter (textbuf, &end);

        if (tag->element_id < ELEMENT_ID_BR) {

            /* If not a self-closing tag... */

            const gchar *s;

            gtk_text_buffer_apply_tag_by_name (textbuf,
                element[tag->element_id].name, &start, &end);

            if ((s = attr_list_get_value (tag->attr, "fg")) != NULL) {

                GdkColor color;

                if (gdk_color_parse (s, &color)) {
                    GtkTextTag *fg;
                    fg = gtk_text_buffer_create_tag
                        (textbuf, NULL, "foreground-gdk", &color, NULL);
                    gtk_text_buffer_apply_tag (textbuf, fg, &start, &end);
                }
                else {
                    g_warning ("bad foreground colour value");
                }
            }

            if ((s = attr_list_get_value (tag->attr, "bg")) != NULL) {

                GdkColor color;

                if (gdk_color_parse (s, &color)) {
                    GtkTextTag *bg;
                    bg = gtk_text_buffer_create_tag
                        (textbuf, NULL, "background-gdk", &color, NULL);
                    gtk_text_buffer_apply_tag (textbuf, bg, &start, &end);
                }
                else {
                    g_warning ("bad background colour value");
                }
            }
        }

        p->tags = g_list_delete_link (p->tags, g_list_first (p->tags));

        mtag_free (tag);
    }
}


static void
cb_audio_play (GtkWidget *widget, Audio *a)
{
    audio_play_uri (a->uri);
}


static void
display_self_close_tag (GtkTextView *textview, GtkTextBuffer *textbuf,
    Parser *p)
{
    GtkTextIter iter;
    GtkTextChildAnchor *anchor;
    GtkWidget *widget;
    const gchar *type, *file;
    gchar *fullpath;


    switch (p->element_id) {
    case ELEMENT_ID_BR:
        textbuf_put (textbuf, "\n");
        break;
    case ELEMENT_ID_EMBED:
        type = attr_list_get_value (p->attr, "type");
        file = attr_list_get_value (p->attr, "src");
        if (type == NULL || file == NULL) {
            /* XXX: Error message */
            return;
        }

        if (g_path_is_absolute (file) || workdir == NULL) {
            fullpath = g_strdup (file);
        }
        else {
            fullpath = g_build_filename (workdir, file, NULL);
        }

        if (strcmp (type, "image") == 0) {
            widget = gtk_image_new_from_file (fullpath);
        }
        else if (strcmp (type, "sound") == 0) {

            Audio *a;

            a = audio_append_file (p->side, workdir, fullpath);
            widget = gtk_button_new_with_mnemonic (_("_Listen"));
            g_signal_connect (G_OBJECT(widget), "clicked",
                G_CALLBACK(cb_audio_play), a);

        }
        else {
            g_free (fullpath);
            return;
        }
        g_free (fullpath);

        gtk_text_buffer_get_end_iter (textbuf, &iter);
        anchor = gtk_text_buffer_create_child_anchor (textbuf, &iter);
        gtk_text_view_add_child_at_anchor (textview, widget, anchor);
        gtk_widget_show_all (widget);
        break;
    default:
        break;
    }
}


void
textview_display_with_markup (Ignuit *ig, GtkTextView *textview,
    const gchar *text, CardStyle style, gint side)
{
    GtkTextBuffer *textbuf;
    Parser *p;
    const gchar *s;
    gchar *ttext;


    workdir = prefs_get_workdir (ig->prefs);

    if (text == NULL) { text = ""; }

    if (side == INFO) {
        ttext = g_strdup (text);
    }
    else {
        gint dpi = prefs_get_latex_dpi (ig->prefs);
        ttext = latex_preprocess (text, workdir, dpi, TRUE);
#if 0
        g_print ("%s\n", ttext);
#endif
    }

    textbuf = gtk_text_view_get_buffer (textview);
    textbuf_clear (textbuf);

    p = parser_new ();

    p->side = side;

    if (style != CARD_STYLE_SENTENCES) {
        p->in_keyword = TRUE;
        p->element_id = ELEMENT_ID_KEYWORD;
        display_open_tag (textbuf, p);
    }

    s = ttext;

    while (parser_get_token (p, s) != TOKEN_NONE) {
        switch (p->token) {
        case TOKEN_TEXT:
            textbuf_put (textbuf, p->data->str);
            break;
        case TOKEN_NEW_LINE:
            if (p->in_keyword) {
                p->in_keyword = FALSE;
                p->element_id = ELEMENT_ID_KEYWORD;
                display_close_tag (textview, textbuf, p);
            }
            textbuf_put (textbuf, p->data->str);
            break;
        case TOKEN_TAG:
            switch (p->tag_type) {
            case TAG_OPEN:
                display_open_tag (textbuf, p);
                break;
            case TAG_CLOSE:
                display_close_tag (textview, textbuf, p);
                break;
            case TAG_SELF_CLOSE:
                display_self_close_tag (textview, textbuf, p);
                attr_list_free (p->attr);
                break;
            }
            break;
        default:
            break;
        }
        s = s + p->length;
    }

    parser_free (p);

    g_free (ttext);
}


#define HIDE_LATEX_FROM_CARD_PANE 0


#if HIDE_LATEX_FROM_CARD_PANE

gchar*
remove_card_markup (const gchar *text)
{
    Parser *p;
    const gchar *s;
    gchar *plain_text, *ttext;
    GString *gstr;


    /* Return a newly allocated string with markup removed. */

    if (text == NULL)
        text = "";

    ttext = latex_preprocess (text, NULL, FALSE);
#if 0
    g_print ("%s\n", ttext);
#endif

    p = parser_new ();

    gstr = g_string_new ("");

    s = ttext;

    while (parser_get_token (p, s) != TOKEN_NONE) {
        switch (p->token) {
        case TOKEN_TEXT:
        case TOKEN_NEW_LINE:
            g_string_append (gstr, p->data->str);
            break;
        case TOKEN_TAG:
            if (p->tag_type == TAG_SELF_CLOSE) {
                if (p->element_id == ELEMENT_ID_BR) {
                    g_string_append_c (gstr, ' ');
                }
                if (p->element_id == ELEMENT_ID_EMBED) {
                    const gchar *alt;
                    alt = attr_list_get_value (p->attr, "alt");
                    if (alt != NULL) {
                        g_string_append (gstr, alt);
                    }
                }
            }
            attr_list_free (p->attr);
            break;
        default:
            break;
        }
        s = s + p->length;
    }

    parser_free (p);

    g_free (ttext);

    plain_text = gstr->str;
    g_string_free (gstr, FALSE);

    return plain_text;
}


#else


gchar*
remove_card_markup (const gchar *text)
{
    Parser *p;
    const gchar *s;
    gchar *plain_text;
    GString *gstr;


    /* Return a newly allocated string with markup removed. */

    p = parser_new ();

    gstr = g_string_new ("");

    s = text;

    while (parser_get_token (p, s) != TOKEN_NONE) {
        switch (p->token) {
        case TOKEN_TEXT:
        case TOKEN_NEW_LINE:
            g_string_append (gstr, p->data->str);
            break;
        case TOKEN_TAG:
            if (p->tag_type == TAG_SELF_CLOSE) {
                if (p->element_id == ELEMENT_ID_BR) {
                    g_string_append_c (gstr, ' ');
                }
                if (p->element_id == ELEMENT_ID_EMBED) {
                    const gchar *alt;
                    alt = attr_list_get_value (p->attr, "alt");
                    if (alt != NULL) {
                        g_string_append (gstr, alt);
                    }
                }
            }
            attr_list_free (p->attr);
            break;
        default:
            break;
        }
        s = s + p->length;
    }

    parser_free (p);

    plain_text = gstr->str;
    g_string_free (gstr, FALSE);

    return plain_text;
}

#endif

