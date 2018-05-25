/* main.h
 *
 * Copyright (C) 2008, 2009, 2016 Timothy Richard Musson
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


#ifndef HAVE_MAIN_H
#define HAVE_MAIN_H


#include "file.h"
#include "prefs.h"

#define GLADE_DIR               PACKAGE G_DIR_SEPARATOR_S "glade"
#define IMPORT_DIR              PACKAGE G_DIR_SEPARATOR_S "import"
#define EXPORT_DIR              PACKAGE G_DIR_SEPARATOR_S "export"

#define F_GLADE_MAIN            GLADE_DIR G_DIR_SEPARATOR_S "main.glade"
#define F_GLADE_EDITOR          GLADE_DIR G_DIR_SEPARATOR_S "editor.glade"
#define F_GLADE_TAGGER          GLADE_DIR G_DIR_SEPARATOR_S "tagger.glade"
#define F_GLADE_QUIZ            GLADE_DIR G_DIR_SEPARATOR_S "quiz.glade"
#define F_GLADE_PROPERTIES      GLADE_DIR G_DIR_SEPARATOR_S "properties.glade"
#define F_GLADE_PREFERENCES     GLADE_DIR G_DIR_SEPARATOR_S "preferences.glade"
#define F_GLADE_FIND            GLADE_DIR G_DIR_SEPARATOR_S "find.glade"
#define F_GLADE_CATEGORY_PROPERTIES      GLADE_DIR G_DIR_SEPARATOR_S "category_properties.glade"

#define F_ICON                  "ignuit.png"
#define F_IMG_BTN_KNOWN         PACKAGE G_DIR_SEPARATOR_S "known.png"
#define F_IMG_BTN_UNKNOWN       PACKAGE G_DIR_SEPARATOR_S "unknown.png"
#define F_IMG_BTN_FLIP          PACKAGE G_DIR_SEPARATOR_S "flip.png"
#define F_IMG_BTN_QUIZ_1        PACKAGE G_DIR_SEPARATOR_S \
                                "box-start-quiz-24.png"

#define UNICHAR_FLAGGED   "\342\232\221"  /* UTF-8 Black Flag */
#define UNICHAR_EXPIRED   "\342\230\205"  /* UTF-8 Black Star */


enum {
    FRONT = 0,
    BACK,
    INFO
};


typedef enum {

    QUIZ_ALL_CATEGORIES = 0,
    QUIZ_CURRENT_CATEGORY,
    QUIZ_N_CATEGORY_SELECTIONS

} QuizSelectCategories;


typedef enum {

    QUIZ_ALL_CARDS = 0,
    QUIZ_NEW_CARDS,
    QUIZ_EXPIRED_CARDS,
    QUIZ_NEW_AND_EXPIRED_CARDS,
    QUIZ_FLAGGED_CARDS,
    QUIZ_SELECTED_CARDS,
    QUIZ_N_CARD_SELECTIONS

} QuizSelectCards;


typedef enum {

    QUIZ_FACE_FRONT = 0,
    QUIZ_FACE_BACK,
    QUIZ_FACE_RANDOM,
    QUIZ_N_FACE_SELECTIONS

} QuizSelectFace;


typedef enum {

    QUIZ_MODE_NORMAL = 0,
    QUIZ_MODE_DRILL

} QuizMode;


typedef struct _QuizInfo QuizInfo;

struct _QuizInfo {

    QuizSelectCategories  category_selection;
    QuizSelectCards       card_selection;
    QuizSelectFace        face_selection;
    gboolean              in_order;

};


typedef struct _Ignuit Ignuit;

struct _Ignuit {

    GnomeProgram *program;

    GDate        *today;
    GRand        *grand;

    File         *file;
    Prefs        *prefs;

    QuizInfo     quizinfo;

    /* Main window */

    GtkWidget    *app;

    GtkTreeView  *treev_cat;
    GtkTreeView  *treev_card;
    GnomeAppBar  *appbar;

    GtkWidget    *m_remove_category;
    GtkWidget    *b_remove_category;
    GtkWidget    *m_category_previous;
    GtkWidget    *m_category_next;
    GtkWidget    *t_category_previous;
    GtkWidget    *t_category_next;
    GtkWidget    *m_category_properties;
    GtkWidget    *m_add_card;
    GtkWidget    *b_add_card;
    GtkWidget    *m_find;
    GtkWidget    *t_find;
    GtkWidget    *m_save;
    GtkWidget    *t_save;
    GtkWidget    *m_start_quiz;
    GtkWidget    *m_start_drill;
    GtkWidget    *t_start_quiz;
    GtkWidget    *m_find_flagged;
    GtkWidget    *m_find_all;
    GtkWidget    *m_view_trash;
    GtkWidget    *m_edit_tags;
    GtkWidget    *m_flag;
    GtkWidget    *m_switch_sides;
    GtkWidget    *m_reset_stats;
    GtkWidget    *m_paste_card;
    GtkWidget    *m_select_all;

    GtkWidget    *m_card_popup_select_all;
    GtkWidget    *m_card_popup_edit_tags;
    GtkWidget    *m_card_popup_flag;
    GtkWidget    *m_card_popup_switch_sides;
    GtkWidget    *m_card_popup_reset_stats;
    GtkWidget    *m_card_popup_paste;

    GtkWidget    *m_category_popup_rename;
    GtkWidget    *m_category_popup_remove;
    GtkWidget    *m_category_popup_toggle_fixed_order;
    GtkWidget    *m_category_popup_properties;

    gchar        *color_expiry;
    gchar        *color_plain;

    GList        *recent_search_terms;

    Category     *clipboard;

    gint         n_cards_selected;
};


gint        get_current_hour (void);
gint        get_current_minute (void);
gboolean    date_today (GDate *date);
gchar*      date_str (gchar *dest, GDate *date);

void        ig_clear_clipboard (Ignuit *ig);
void        ig_add_clipboard (Ignuit *ig, Card *c);
Category*   ig_get_clipboard (Ignuit *ig);
gboolean    ig_category_is_clipboard (Ignuit *ig, Category *cat);
void        ig_file_changed (Ignuit *ig);


#endif /* HAVE_MAIN_H */

