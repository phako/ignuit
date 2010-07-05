/* audio.h
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


#ifndef HAVE_AUDIO_H
#define HAVE_AUDIO_H


typedef struct _Audio Audio;

struct _Audio {
    gint    side;
    gchar   *uri;
};

Audio*          audio_append_file (gint side, const gchar *workdir,
                    const gchar *fname);
const gchar*    audio_get_fname (Audio *a);
void            audio_free_all (void);
void            audio_play_side (gint side);

void            audio_play_uri (const gchar *uri);
void            audio_stop (void);


#endif /* HAVE_AUDIO_H */

