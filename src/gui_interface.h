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

#ifndef _GUI_INTERFACE_H
#define _GUI_INTERFACE_H

#include <gtk/gtk.h>

extern GtkWidget *main_window;
extern GtkWidget *open_file_selection;
extern GtkWidget *save_file_selection;

extern GtkWidget *about_window;
extern GtkWidget *about_label;

extern GtkWidget *open_file_position_window;
extern GtkObject *open_file_position_spin_adj;
extern GtkWidget *open_file_position_name_label;

extern GtkWidget *save_file_range_window;
extern GtkObject *save_file_start_spin_adj;
extern GtkWidget *save_file_start_name;
extern GtkObject *save_file_end_spin_adj;
extern GtkWidget *save_file_end_name;

extern GtkWidget *edit_save_position_window;
extern GtkObject *edit_save_position_spin_adj;
extern GtkWidget *edit_save_position_name_label;

extern GtkWidget *notice_window;
extern GtkWidget *notice_label_1;
extern GtkWidget *notice_label_2;

extern GtkWidget *patches_clist;

extern GtkWidget *osc1_waveform_pixmap;
extern GtkWidget *osc2_waveform_pixmap;
extern GtkWidget *lfo_waveform_pixmap;

extern GtkWidget *name_entry;

extern GtkObject *tuning_adj;
extern GtkObject *polyphony_adj;
extern GtkWidget *monophonic_option_menu;
extern GtkWidget *glide_option_menu;
extern GtkObject *bendrange_adj;

extern GtkObject *voice_widget[];

void create_windows(const char *instance_tag);

#endif /* _GUI_INTERFACE_H */

