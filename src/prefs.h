/* prefs.h
 *
 * Copyright (C) 2008, 2009, 2017 Timothy Richard Musson
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


#ifndef HAVE_PREFS_H
#define HAVE_PREFS_H


#define PREF_APP_WIDTH              "/apps/ignuit/app_width"
#define PREF_APP_HEIGHT             "/apps/ignuit/app_height"
#define PREF_CATEGORY_PANE_WIDTH    "/apps/ignuit/category_pane_width"
#define PREF_QUIZ_WIDTH             "/apps/ignuit/quiz_width"
#define PREF_QUIZ_HEIGHT            "/apps/ignuit/quiz_height"
#define PREF_TAGGER_WIDTH           "/apps/ignuit/tagger_width"
#define PREF_TAGGER_HEIGHT          "/apps/ignuit/tagger_height"
#define PREF_CATEGORY_COLUMN_TITLE_WIDTH "/apps/ignuit/category_column_title_width"
#define PREF_CARD_COLUMN_FRONT_WIDTH    "/apps/ignuit/card_column_front_width"
#define PREF_CARD_COLUMN_BACK_WIDTH     "/apps/ignuit/card_column_back_width"
#define PREF_CARD_COLUMN_CATEGORY_WIDTH "/apps/ignuit/card_column_category_width"
#define PREF_CARD_COLUMN_VISIBLE    "/apps/ignuit/card_column_visible"
#define PREF_STICKY_FLIPS           "/apps/ignuit/sticky_flips"
#define PREF_AUTO_LISTEN            "/apps/ignuit/auto_listen"
#define PREF_CARD_FONT              "/apps/ignuit/card_font"
#define PREF_CARD_COLORS            "/apps/ignuit/card_colors"
#define PREF_SCHEDULES              "/apps/ignuit/schedules"
#define PREF_WORKDIR                "/apps/ignuit/workdir"
#define PREF_FIND_WITH_REGEX        "/apps/ignuit/find_with_regex"
#define PREF_CONFIRM_EMPTY_TRASH    "/apps/ignuit/confirm_empty_trash"
#define PREF_MAIN_TOOLBAR_VISIBLE   "/apps/ignuit/main_toolbar_visible"
#define PREF_CATEGORY_PANE_VISIBLE  "/apps/ignuit/category_pane_visible"
#define PREF_BOTTOM_TOOLBAR_VISIBLE "/apps/ignuit/bottom_toolbar_visible"
#define PREF_STATUSBAR_VISIBLE      "/apps/ignuit/statusbar_visible"
#define PREF_QUIZ_ANSWERBAR_VISIBLE "/apps/ignuit/quiz_answerbar_visible"
#define PREF_EDITOR_TAGBAR_VISIBLE  "/apps/ignuit/editor_tagbar_visible"
#define PREF_LATEX_DPI              "/apps/ignuit/latex_dpi"
#define PREF_BACKUP                 "/apps/ignuit/backup"

#define DEFAULT_APP_WIDTH           600
#define DEFAULT_APP_HEIGHT          300
#define DEFAULT_CATEGORY_PANE_WIDTH 225
#define DEFAULT_QUIZ_WIDTH          450
#define DEFAULT_QUIZ_HEIGHT         250
#define DEFAULT_TAGGER_WIDTH        400
#define DEFAULT_TAGGER_HEIGHT       300
#define DEFAULT_CARD_COLUMN_FRONT_WIDTH     200
#define DEFAULT_CARD_COLUMN_BACK_WIDTH      200
#define DEFAULT_CARD_COLUMN_CATEGORY_WIDTH  150
#define DEFAULT_CARD_COLUMN_VISIBLE "front,group,tested,expired,expiry,flagged"
#define DEFAULT_CATEGORY_COLUMN_TITLE_WIDTH   200
#define DEFAULT_SCHEDULES           "1,3,7,20,55,145,400,1000,1000"
#define DEFAULT_CARD_COLORS         "#000000,#FFFDCC,#C5D2C8,#E0B6AF,#D0D0D0,#E36540"
#define DEFAULT_CARD_FONT           "Sans 11"
#define DEFAULT_LATEX_DPI           150

#define MAX_LATEX_DPI               500


typedef struct _Prefs Prefs;


typedef enum {
    COLOR_CARD_FG = 0,
    COLOR_CARD_BG,
    COLOR_CARD_BG_KNOWN,
    COLOR_CARD_BG_UNKNOWN,
    COLOR_CARD_BG_END,
    COLOR_CARD_EXPIRED,
    N_CARD_COLORS
} Color;

typedef enum {
    LISTEN_NONE  = 0,
    LISTEN_FRONT = 1,
    LISTEN_BACK  = 2
} AutoListen;


Prefs*          prefs_load (void);
void            prefs_free (Prefs *p);

void            prefs_set_backup (Prefs *p, gboolean backup);
gboolean        prefs_get_backup (Prefs *p);

Group           prefs_get_schedule (Prefs *p, Group g);
void            prefs_set_schedule (Prefs *p, Group g, gint days);
void            prefs_csvstr_to_schedules (Prefs *p, const gchar *csvstr);

void            prefs_set_sticky_flips (Prefs *p, gboolean sticky);
gboolean        prefs_get_sticky_flips (Prefs *p);

void            prefs_set_auto_listen (Prefs *p, AutoListen listen);
AutoListen      prefs_get_auto_listen (Prefs *p);

void            prefs_set_category_pane_width (Prefs *p, gint width);
gint            prefs_get_category_pane_width (Prefs *p);

void            prefs_set_main_toolbar_visible (Prefs *p, gboolean visible);
gboolean        prefs_get_main_toolbar_visible (Prefs *p);
void            prefs_set_category_pane_visible (Prefs *p, gboolean visible);
gboolean        prefs_get_category_pane_visible (Prefs *p);
void            prefs_set_bottom_toolbar_visible (Prefs *p, gboolean visible);
gboolean        prefs_get_bottom_toolbar_visible (Prefs *p);
void            prefs_set_statusbar_visible (Prefs *p, gboolean visible);
gboolean        prefs_get_statusbar_visible (Prefs *p);
void            prefs_set_quiz_answer_bar_visible (Prefs *p, gboolean visible);
gboolean        prefs_get_quiz_answer_bar_visible (Prefs *p);
void            prefs_set_editor_tag_bar_visible (Prefs *p, gboolean visible);
gboolean        prefs_get_editor_tag_bar_visible (Prefs *p);

void            prefs_set_color (Prefs *p, Color which, const gchar *spec);
void            prefs_set_color_gdk (Prefs *p, Color which, GdkColor *color);
GdkColor*       prefs_get_color (Prefs *p, Color which);
void            prefs_csvstr_to_colors (Prefs *p, const gchar *csvstr);

void            prefs_set_app_size (Prefs *p, gint width, gint height);
gint            prefs_get_app_width (Prefs *p);
gint            prefs_get_app_height (Prefs *p);

void            prefs_set_category_column_title_width (Prefs *p, gint width);
gint            prefs_get_category_column_title_width (Prefs *p);

void            prefs_set_card_column_front_width (Prefs *p, gint width);
gint            prefs_get_card_column_front_width (Prefs *p);

void            prefs_set_card_column_back_width (Prefs *p, gint width);
gint            prefs_get_card_column_back_width (Prefs *p);

void            prefs_set_card_column_category_width (Prefs *p, gint width);
gint            prefs_get_card_column_category_width (Prefs *p);

void            prefs_set_card_column_visible (Prefs *p, gint col, gboolean visible);
gboolean        prefs_get_card_column_visible (Prefs *p, gint col);

void            prefs_set_quiz_size (Prefs *p, gint width, gint height);
gint            prefs_get_quiz_width (Prefs *p);
gint            prefs_get_quiz_height (Prefs *p);

void            prefs_set_tagger_size (Prefs *p, gint height, gint width);
gint            prefs_get_tagger_width (Prefs *p);
gint            prefs_get_tagger_height (Prefs *p);

void            prefs_set_card_font (Prefs *p, const gchar *fontname);
const gchar*    prefs_get_card_font (Prefs *p);

void            prefs_set_workdir (Prefs *p, const gchar *dirname);
void            prefs_set_workdir_from_filename (Prefs *p, const gchar *fname);
const gchar*    prefs_get_workdir (Prefs *p);

void            prefs_set_find_with_regex (Prefs *p, gboolean use_regex);
gboolean        prefs_get_find_with_regex (Prefs *p);

void            prefs_set_confirm_empty_trash (Prefs *p, gboolean confirm);
gboolean        prefs_get_confirm_empty_trash (Prefs *p);

void            prefs_set_latex_dpi (Prefs *p, gint dpi);
gint            prefs_get_latex_dpi (Prefs *p);


#endif /* HAVE_PREFS_H */

