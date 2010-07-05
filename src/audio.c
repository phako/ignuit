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
#include <glib.h>
#include <gst/gst.h>

#include "main.h"
#include "audio.h"
#include "dialog-quiz.h"  /* XXX: Get rid of this include. */


static GMainLoop *loop = NULL;


static GList *audio = NULL;


static Audio*
audio_new (gint side, const gchar *workdir, const gchar *fname)
{
    Audio *a;

    a = g_new0 (Audio, 1);
    a->side = side;

    if (g_path_is_absolute (fname)) {
        a->uri = g_strdup_printf ("file://%s", fname);
    }
    else if (workdir != NULL) {
        a->uri = g_strdup_printf ("file://%s/%s", workdir, fname);
    }

    return a;
}


static void
audio_free (Audio *a)
{
#if 0
    g_printerr ("audio free: %s\n", a->uri);
#endif
    g_free (a->uri);
    g_free (a);
}


const gchar*
audio_get_uri (Audio *a)
{
    return a->uri;
}


void
audio_free_all (void)
{
    if (audio != NULL) {

        GList *cur;

        for (cur = audio; cur != NULL; cur = cur->next)
            audio_free ((Audio*)cur->data);

        g_list_free (audio);

        audio = NULL;
    }
}


void
audio_play_side (gint side)
{
    Audio *a;
    GList *cur;

    /* Play each audio file on the specified side. */

    for (cur = audio; cur != NULL; cur = cur->next) {

        a = (Audio*)cur->data;
        if (a->side == side)
            audio_play_uri (a->uri);

    }
}


Audio*
audio_append_file (gint side, const gchar *workdir, const gchar *fname)
{
    audio = g_list_prepend (audio, audio_new (side, workdir, fname));
    return audio->data;
}


/* The functions cb_bus and play_audio are taken pretty much
 * directly from GStreamer Application Development Manual (0.10.20.1)
 */

static gboolean
cb_bus (GstBus *bus, GstMessage *message, gpointer data)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        {
        GError *err;
        gchar *debug;

        gst_message_parse_error (message, &err, &debug);
        g_warning ("play_audio: error: %s\n", err->message);
        g_error_free (err);
        g_free (debug);

        g_main_loop_quit (loop);
        break;
        }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        g_main_loop_quit (loop);
        break;
    default:
        break;
    }

    return TRUE;
}


void
audio_play_uri (const gchar *uri)
{
    GstElement *play;
    GstBus *bus;


    if (uri == NULL)
        return;

    if (loop != NULL && g_main_loop_is_running (loop))
        return;

    dialog_quiz_set_sensitive (FALSE); /* XXX */

    loop = g_main_loop_new (NULL, FALSE);

    play = gst_element_factory_make ("playbin", "play");
    g_object_set (G_OBJECT(play), "uri", uri, NULL);

    bus = gst_pipeline_get_bus (GST_PIPELINE(play));
    gst_bus_add_watch (bus, cb_bus, loop);
    gst_object_unref (bus);

    gst_element_set_state (play, GST_STATE_PLAYING);

    g_main_loop_run (loop);
    loop = NULL;

    gst_element_set_state (play, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (play));

    dialog_quiz_set_sensitive (TRUE); /* XXX */
}


void
audio_stop (void)
{
    if (loop == NULL)
        return;

    g_main_loop_quit (loop);
    loop = NULL;
}

