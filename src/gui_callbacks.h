/* Xsynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#ifndef _GUI_CALLBACKS_H
#define _GUI_CALLBACKS_H

#include <gtk/gtk.h>

#include "xsynth_types.h"

void on_menu_open_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_save_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_about_activate(GtkMenuItem *menuitem, gpointer user_data);
gint on_delete_event_wrapper(GtkWidget *widget, GdkEvent *event,
                             gpointer data);
void on_open_file_ok(GtkWidget *widget, gpointer data);
void on_open_file_cancel(GtkWidget *widget, gpointer data);
void on_position_change(GtkWidget *widget, gpointer data);
void on_open_file_position_ok(GtkWidget *widget, gpointer data);
void on_open_file_position_cancel(GtkWidget *widget, gpointer data);
void on_save_file_ok(GtkWidget *widget, gpointer data);
void on_save_file_cancel(GtkWidget *widget, gpointer data);
void on_save_file_range_change(GtkWidget *widget, gpointer data);
void on_save_file_range_ok(GtkWidget *widget, gpointer data);
void on_save_file_range_cancel(GtkWidget *widget, gpointer data);
void on_about_dismiss(GtkWidget *widget, gpointer data);
void on_notebook_switch_page(GtkNotebook *notebook, GtkNotebookPage *page,
                             guint page_num);
void on_patches_selection(GtkWidget *clist, gint row, gint column,
                          GdkEventButton *event, gpointer data);
void on_voice_slider_change(GtkWidget *widget, gpointer data);
void on_voice_detent_change(GtkWidget *widget, gpointer data);
void on_voice_onoff_toggled(GtkWidget *widget, gpointer data);
void on_vcf_mode_activate(GtkWidget *widget, gpointer data);
void on_test_note_slider_change(GtkWidget *widget, gpointer data);
void on_test_note_button_press(GtkWidget *widget, gpointer data);
void on_edit_action_button_press(GtkWidget *widget, gpointer data);
void on_edit_save_position_ok(GtkWidget *widget, gpointer data);
void on_edit_save_position_cancel(GtkWidget *widget, gpointer data);
void on_tuning_change(GtkWidget *widget, gpointer data);
void on_polyphony_change(GtkWidget *widget, gpointer data);
void on_mono_mode_activate(GtkWidget *widget, gpointer data);
void on_glide_mode_activate(GtkWidget *widget, gpointer data);
void on_bendrange_change(GtkWidget *widget, gpointer data);
void display_notice(char *message1, char *message2);
void on_notice_dismiss(GtkWidget *widget, gpointer data);
void update_detent_label(int index, int value);
void update_voice_widget(int port, float value);
void update_voice_widgets_from_patch(xsynth_patch_t *patch);
void update_from_program_select(int bank, int program);
void update_patch_from_voice_widgets(xsynth_patch_t *patch);
void update_patches(const char *key, const char *value);
void update_polyphony(const char *value);
void update_monophonic(const char *value);
void update_glide(const char *value);
void update_bendrange(const char *value);
void rebuild_patches_clist(void);

#endif  /* _GUI_CALLBACKS_H */

