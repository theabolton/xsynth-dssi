/* Xsynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004 Sean Bolton
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#ifdef HAVE_PHAT
#include <phat/phat.h>
#define GET_VOICE_SLIDER_ADJUSTMENT(x) (phat_fan_slider_get_adjustment(PHAT_FAN_SLIDER(x)))
#else
#define GET_VOICE_SLIDER_ADJUSTMENT(x) (gtk_range_get_adjustment(GTK_RANGE(x)))
#endif

#include "xsynth_ports.h"
#include "xsynth_synth.h"
#include "gui_callbacks.h"
#include "gui_interface.h"
#include "gui_images.h"

GtkWidget *main_window;
GtkWidget *open_file_selection;
GtkWidget *save_file_selection;

GtkWidget *about_window;
GtkWidget *about_label;

GtkWidget *open_file_position_window;
GtkObject *open_file_position_spin_adj;
GtkWidget *open_file_position_name_label;

GtkWidget *edit_save_position_window;
GtkObject *edit_save_position_spin_adj;
GtkWidget *edit_save_position_name_label;

GtkWidget *notice_window;
GtkWidget *notice_label_1;
GtkWidget *notice_label_2;

GtkWidget *patches_clist;

GtkWidget *osc1_waveform_label;
GtkWidget *osc2_waveform_label;
GtkWidget *lfo_waveform_label;

GtkWidget *name_entry;

GtkWidget *monophonic_option_menu;
GtkObject *polyphony_adj;

GtkObject *voice_widget[XSYNTH_PORTS_COUNT];

/*
 * create_voice_slider
 *
 * create a patch edit slider
 *
 * example:
 *    osc1_pitch = create_voice_slider(main_window, "osc1_pitch",
 *                                     XSYNTH_PORT_OSC1_PITCH);
 */
static GtkWidget *
create_voice_slider(GtkWidget *main_window, const char *name, int index)
{
#ifdef HAVE_PHAT
    GtkObject *adjustment = gtk_adjustment_new (0, 0, 10, 0.005, 1, 1);
    GtkWidget *scale = phat_hfan_slider_new (GTK_ADJUSTMENT (adjustment));
#else
    GtkObject *adjustment = gtk_adjustment_new (0, 0, 11, 0.005, 1, 1);
    GtkWidget *scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
    gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
#endif

    voice_widget[index] = adjustment;
    gtk_widget_ref (scale);
    gtk_object_set_data_full (GTK_OBJECT (main_window), name, scale,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (scale);
    /* gtk_widget_set_usize(scale, 200, -1); // blinking: doesn't help.  */
    return scale;
}

void
create_main_window (const char *tag)
{
  GtkWidget *vbox1;
  GtkWidget *menubar1;
  GtkWidget *file1;
  GtkWidget *file1_menu;
#if !GTK_CHECK_VERSION(2, 0, 0)
  GtkAccelGroup *file1_menu_accels;
  GtkAccelGroup *help1_menu_accels;
#endif
  GtkWidget *menu_open;
  GtkWidget *menu_save;
  GtkWidget *separator1;
  GtkWidget *menu_quit;
  GtkWidget *help1;
  GtkWidget *help1_menu;
  GtkWidget *menu_about;
  GtkWidget *notebook1;
  GtkWidget *scrolledwindow1;
  GtkWidget *label45;
  GtkWidget *label46;
  GtkWidget *patches_tab_label;
  GtkWidget *patch_edit_table;
  GtkWidget *frame2;
  GtkWidget *table5;
  GtkWidget *label13;
  GtkWidget *label14;
  GtkWidget *label15;
    GtkWidget *hbox1;
    GtkObject *osc1_waveform_adj;
    GtkWidget *osc1_waveform_spin;
  GtkWidget *osc1_pulsewidth;
  GtkWidget *osc1_pitch;
  GtkWidget *frame3;
  GtkWidget *table6;
  GtkWidget *label16;
  GtkWidget *label17;
  GtkWidget *label18;
  GtkWidget *label19;
    GtkWidget *hbox2;
    GtkObject *osc2_waveform_adj;
    GtkWidget *osc2_waveform_spin;
  GtkWidget *osc2_pulsewidth;
  GtkWidget *osc2_pitch;
  GtkWidget *osc_sync;
  GtkWidget *frame4;
  GtkWidget *table7;
  GtkWidget *label20;
  GtkWidget *label21;
  GtkWidget *label22;
  GtkWidget *label23;
    GtkWidget *hbox3;
    GtkObject *lfo_waveform_adj;
    GtkWidget *lfo_waveform_spin;
  GtkWidget *lfo_frequency;
  GtkWidget *lfo_amount_o;
  GtkWidget *lfo_amount_f;
  GtkWidget *frame5;
  GtkWidget *table8;
  GtkWidget *label24;
  GtkWidget *osc_balance;
  GtkWidget *frame6;
  GtkWidget *table9;
  GtkWidget *label25;
  GtkWidget *glide_time;
  GtkWidget *frame7;
  GtkWidget *table10;
  GtkWidget *label26;
  GtkWidget *label27;
  GtkWidget *label28;
  GtkWidget *label29;
  GtkWidget *label30;
  GtkWidget *label31;
  GtkWidget *eg1_attack;
  GtkWidget *eg1_decay;
  GtkWidget *eg1_sustain;
  GtkWidget *eg1_release;
  GtkWidget *eg1_amount_o;
  GtkWidget *eg1_amount_f;
  GtkWidget *frame8;
  GtkWidget *table11;
  GtkWidget *label32;
  GtkWidget *label33;
  GtkWidget *label34;
  GtkWidget *label35;
  GtkWidget *label36;
  GtkWidget *label37;
  GtkWidget *eg2_attack;
  GtkWidget *eg2_amount_f;
  GtkWidget *eg2_amount_o;
  GtkWidget *eg2_release;
  GtkWidget *eg2_sustain;
  GtkWidget *eg2_decay;
  GtkWidget *frame9;
  GtkWidget *table12;
  GtkWidget *label38;
  GtkWidget *label39;
  GtkWidget *label40;
  GtkWidget *vcf_4pole;
  GtkWidget *vcf_cutoff;
  GtkWidget *vcf_qres;
  GtkWidget *frame10;
  GtkWidget *table13;
  GtkWidget *label41;
  GtkWidget *volume;
  GtkWidget *frame11;
  GtkWidget *table14;
  GtkWidget *label42;
  GtkWidget *frame12;
  GtkWidget *table15;
  GtkWidget *label10;
  GtkWidget *test_note_button;
  GtkWidget *test_note_key;
  GtkWidget *test_note_velocity;
  GtkWidget *frame13;
  GtkWidget *table17;
  GtkWidget *save_changes;
  GtkWidget *discard_changes;
  GtkWidget *label51;
  GtkWidget *label52;
  GtkWidget *label53;
  GtkWidget *label54;
  GtkWidget *eg1_vel_sens;
  GtkWidget *eg2_vel_sens;
  GtkWidget *patch_edit_tab_label;
  GtkWidget *frame14;
  GtkWidget *table16;
  GtkObject *tuning_spin_adj;
  GtkWidget *tuning_spin;
  GtkWidget *mono_mode_off;
  GtkWidget *mono_mode_on;
  GtkWidget *mono_mode_once;
  GtkWidget *mono_mode_both;
  GtkWidget *optionmenu5_menu;
  GtkWidget *polyphony;
  GtkWidget *label43;
  GtkWidget *label44;
  GtkWidget *frame15;
  GtkWidget *logo_pixmap;
  GtkWidget *label47;
  GtkWidget *configuration_tab_label;
  GtkWidget *statusbar1;
  GtkAccelGroup *accel_group;
    int i;

  accel_group = gtk_accel_group_new ();

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (main_window), "main_window", main_window);
  gtk_window_set_title (GTK_WINDOW (main_window), tag);
  gtk_widget_realize(main_window);  /* window must be realized for create_logo_pixmap() */

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (main_window), vbox1);

  menubar1 = gtk_menu_bar_new ();
  gtk_widget_ref (menubar1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menubar1", menubar1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menubar1);
  gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

  file1 = gtk_menu_item_new_with_label ("File");
  gtk_widget_ref (file1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "file1", file1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file1);
  gtk_container_add (GTK_CONTAINER (menubar1), file1);

  file1_menu = gtk_menu_new ();
  gtk_widget_ref (file1_menu);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "file1_menu", file1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file1), file1_menu);
#if !GTK_CHECK_VERSION(2, 0, 0)
  file1_menu_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (file1_menu));
#endif

  menu_open = gtk_menu_item_new_with_label ("Open Patch Bank...");
  gtk_widget_ref (menu_open);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_open", menu_open,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_open);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_open);
  gtk_widget_add_accelerator (menu_open, "activate", accel_group,
                              GDK_O, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  menu_save = gtk_menu_item_new_with_label ("Save Patch Bank...");
  gtk_widget_ref (menu_save);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_save", menu_save,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_save);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_save);
  gtk_widget_add_accelerator (menu_save, "activate", accel_group,
                              GDK_S, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  separator1 = gtk_menu_item_new ();
  gtk_widget_ref (separator1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "separator1", separator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (separator1);
  gtk_container_add (GTK_CONTAINER (file1_menu), separator1);
  gtk_widget_set_sensitive (separator1, FALSE);

  menu_quit = gtk_menu_item_new_with_label ("Quit");
  gtk_widget_ref (menu_quit);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_quit", menu_quit,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_quit);
  gtk_container_add (GTK_CONTAINER (file1_menu), menu_quit);
  gtk_widget_add_accelerator (menu_quit, "activate", accel_group,
                              GDK_Q, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  help1 = gtk_menu_item_new_with_label ("Help");
  gtk_widget_ref (help1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "help1", help1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (help1);
  gtk_container_add (GTK_CONTAINER (menubar1), help1);
  gtk_menu_item_right_justify (GTK_MENU_ITEM (help1));

  help1_menu = gtk_menu_new ();
  gtk_widget_ref (help1_menu);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "help1_menu", help1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (help1), help1_menu);
#if !GTK_CHECK_VERSION(2, 0, 0)
  help1_menu_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (help1_menu));
#endif

  menu_about = gtk_menu_item_new_with_label ("About");
  gtk_widget_ref (menu_about);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_about", menu_about,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menu_about);
  gtk_container_add (GTK_CONTAINER (help1_menu), menu_about);

  notebook1 = gtk_notebook_new ();
  gtk_widget_ref (notebook1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "notebook1", notebook1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notebook1);
  gtk_box_pack_start (GTK_BOX (vbox1), notebook1, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_container_add (GTK_CONTAINER (notebook1), scrolledwindow1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  patches_clist = gtk_clist_new (2);
  gtk_widget_ref (patches_clist);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "patches_clist", patches_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (patches_clist);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), patches_clist);
  gtk_clist_set_column_width (GTK_CLIST (patches_clist), 0, 50);
  gtk_clist_set_column_width (GTK_CLIST (patches_clist), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (patches_clist));

  label45 = gtk_label_new ("Prog No");
  gtk_widget_ref (label45);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label45", label45,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label45);
  gtk_clist_set_column_widget (GTK_CLIST (patches_clist), 0, label45);

  label46 = gtk_label_new ("Name");
  gtk_widget_ref (label46);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label46", label46,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label46);
  gtk_clist_set_column_widget (GTK_CLIST (patches_clist), 1, label46);

  patches_tab_label = gtk_label_new ("Patches");
  gtk_widget_ref (patches_tab_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "patches_tab_label", patches_tab_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (patches_tab_label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), patches_tab_label);

    /* Patch Edit tab */
    for (i = 0; i < XSYNTH_PORTS_COUNT; i++) voice_widget[i] = NULL;

  patch_edit_table = gtk_table_new (7, 2, FALSE);
  gtk_widget_ref (patch_edit_table);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "patch_edit_table", patch_edit_table,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (patch_edit_table);
  gtk_container_add (GTK_CONTAINER (notebook1), patch_edit_table);
  gtk_container_set_border_width (GTK_CONTAINER (patch_edit_table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (patch_edit_table), 7);
  gtk_table_set_col_spacings (GTK_TABLE (patch_edit_table), 7);

  frame2 = gtk_frame_new ("VCO1");
  gtk_widget_ref (frame2);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame2", frame2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame2);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame2, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  table5 = gtk_table_new (3, 2, FALSE);
  gtk_widget_ref (table5);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table5", table5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table5);
  gtk_container_add (GTK_CONTAINER (frame2), table5);
  gtk_container_set_border_width (GTK_CONTAINER (table5), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table5), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table5), 5);

  label13 = gtk_label_new ("pitch");
  gtk_widget_ref (label13);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label13", label13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label13);
  gtk_table_attach (GTK_TABLE (table5), label13, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label13), 0, 0.5);

  label14 = gtk_label_new ("waveform");
  gtk_widget_ref (label14);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label14", label14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label14);
  gtk_table_attach (GTK_TABLE (table5), label14, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label14), 0, 0.5);

  label15 = gtk_label_new ("pulsewidth");
  gtk_widget_ref (label15);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label15", label15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label15);
  gtk_table_attach (GTK_TABLE (table5), label15, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label15), 0, 0.5);

  hbox1 = gtk_hbox_new (FALSE, 10);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_table_attach (GTK_TABLE (table5), hbox1, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

    osc1_waveform_adj = gtk_adjustment_new (0, 0, 5, 1, 1, 1);
    voice_widget[XSYNTH_PORT_OSC1_WAVEFORM] = osc1_waveform_adj;
  osc1_waveform_spin = gtk_spin_button_new (GTK_ADJUSTMENT (osc1_waveform_adj), 1, 0);
  gtk_widget_ref (osc1_waveform_spin);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "osc1_waveform_spin", osc1_waveform_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (osc1_waveform_spin);
  gtk_box_pack_start (GTK_BOX (hbox1), osc1_waveform_spin, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (osc1_waveform_spin), TRUE);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (osc1_waveform_spin), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (osc1_waveform_spin), TRUE);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (osc1_waveform_spin), TRUE);
    
  osc1_waveform_label = gtk_label_new ("sawtooth down");
  gtk_widget_ref (osc1_waveform_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "osc1_waveform_label", osc1_waveform_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (osc1_waveform_label);
  gtk_box_pack_start (GTK_BOX (hbox1), osc1_waveform_label, FALSE, FALSE, 2);
  gtk_label_set_justify (GTK_LABEL (osc1_waveform_label), GTK_JUSTIFY_LEFT);

    osc1_pitch = create_voice_slider(main_window, "osc1_pitch",
                                     XSYNTH_PORT_OSC1_PITCH);
    gtk_table_attach (GTK_TABLE (table5), osc1_pitch, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    osc1_pulsewidth = create_voice_slider(main_window, "osc1_pulsewidth",
                                          XSYNTH_PORT_OSC1_PULSEWIDTH);
    gtk_table_attach (GTK_TABLE (table5), osc1_pulsewidth, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame3 = gtk_frame_new ("VCO2");
  gtk_widget_ref (frame3);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame3", frame3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame3);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame3, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table6 = gtk_table_new (4, 2, FALSE);
  gtk_widget_ref (table6);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table6", table6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table6);
  gtk_container_add (GTK_CONTAINER (frame3), table6);
  gtk_container_set_border_width (GTK_CONTAINER (table6), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table6), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table6), 5);

  label16 = gtk_label_new ("pitch");
  gtk_widget_ref (label16);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label16", label16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label16);
  gtk_table_attach (GTK_TABLE (table6), label16, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label16), 0, 0.5);

  label17 = gtk_label_new ("waveform");
  gtk_widget_ref (label17);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label17", label17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label17);
  gtk_table_attach (GTK_TABLE (table6), label17, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label17), 0, 0.5);

  label18 = gtk_label_new ("pulsewidth");
  gtk_widget_ref (label18);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label18", label18,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label18);
  gtk_table_attach (GTK_TABLE (table6), label18, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label18), 0, 0.5);

  label19 = gtk_label_new ("oscillator sync");
  gtk_widget_ref (label19);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label19", label19,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label19);
  gtk_table_attach (GTK_TABLE (table6), label19, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label19), 0, 0.5);

    osc_sync = gtk_check_button_new ();
    voice_widget[XSYNTH_PORT_OSC_SYNC] = (GtkObject *)osc_sync;
  gtk_widget_ref (osc_sync);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "osc_sync", osc_sync,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (osc_sync);
  gtk_table_attach (GTK_TABLE (table6), osc_sync, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  hbox2 = gtk_hbox_new (FALSE, 10);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "hbox2", hbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_table_attach (GTK_TABLE (table6), hbox2, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

    osc2_waveform_adj = gtk_adjustment_new (0, 0, 5, 1, 1, 1);
    voice_widget[XSYNTH_PORT_OSC2_WAVEFORM] = osc2_waveform_adj;
  osc2_waveform_spin = gtk_spin_button_new (GTK_ADJUSTMENT (osc2_waveform_adj), 1, 0);
  gtk_widget_ref (osc2_waveform_spin);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "osc2_waveform_spin", osc2_waveform_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (osc2_waveform_spin);
  gtk_box_pack_start (GTK_BOX (hbox2), osc2_waveform_spin, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (osc2_waveform_spin), TRUE);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (osc2_waveform_spin), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (osc2_waveform_spin), TRUE);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (osc2_waveform_spin), TRUE);
    
  osc2_waveform_label = gtk_label_new ("sawtooth down");
  gtk_widget_ref (osc2_waveform_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "osc2_waveform_label", osc2_waveform_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (osc2_waveform_label);
  gtk_box_pack_start (GTK_BOX (hbox2), osc2_waveform_label, FALSE, FALSE, 2);
  gtk_label_set_justify (GTK_LABEL (osc2_waveform_label), GTK_JUSTIFY_LEFT);

    osc2_pitch = create_voice_slider(main_window, "osc2_pitch",
                                     XSYNTH_PORT_OSC2_PITCH);
    gtk_table_attach (GTK_TABLE (table6), osc2_pitch, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    osc2_pulsewidth = create_voice_slider(main_window, "osc2_pulsewidth",
                                          XSYNTH_PORT_OSC2_PULSEWIDTH);
    gtk_table_attach (GTK_TABLE (table6), osc2_pulsewidth, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame4 = gtk_frame_new ("LFO");
  gtk_widget_ref (frame4);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame4", frame4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame4);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame4, 0, 1, 1, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  table7 = gtk_table_new (4, 2, FALSE);
  gtk_widget_ref (table7);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table7", table7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table7);
  gtk_container_add (GTK_CONTAINER (frame4), table7);
  gtk_container_set_border_width (GTK_CONTAINER (table7), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table7), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table7), 5);

  label20 = gtk_label_new ("lfo frequency");
  gtk_widget_ref (label20);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label20", label20,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label20);
  gtk_table_attach (GTK_TABLE (table7), label20, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label20), 0, 0.5);

  label21 = gtk_label_new ("waveform");
  gtk_widget_ref (label21);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label21", label21,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label21);
  gtk_table_attach (GTK_TABLE (table7), label21, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label21), 0, 0.5);

  label22 = gtk_label_new ("pitch depth");
  gtk_widget_ref (label22);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label22", label22,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label22);
  gtk_table_attach (GTK_TABLE (table7), label22, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label22), 0, 0.5);

  label23 = gtk_label_new ("filter depth");
  gtk_widget_ref (label23);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label23", label23,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label23);
  gtk_table_attach (GTK_TABLE (table7), label23, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label23), 0, 0.5);

  hbox3 = gtk_hbox_new (FALSE, 10);
  gtk_widget_ref (hbox3);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "hbox3", hbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox3);
  gtk_table_attach (GTK_TABLE (table7), hbox3, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

    lfo_waveform_adj = gtk_adjustment_new (0, 0, 5, 1, 1, 1);
    voice_widget[XSYNTH_PORT_LFO_WAVEFORM] = lfo_waveform_adj;
  lfo_waveform_spin = gtk_spin_button_new (GTK_ADJUSTMENT (lfo_waveform_adj), 1, 0);
  gtk_widget_ref (lfo_waveform_spin);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "lfo_waveform_spin", lfo_waveform_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (lfo_waveform_spin);
  gtk_box_pack_start (GTK_BOX (hbox3), lfo_waveform_spin, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (lfo_waveform_spin), TRUE);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (lfo_waveform_spin), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (lfo_waveform_spin), TRUE);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (lfo_waveform_spin), TRUE);
    
  lfo_waveform_label = gtk_label_new ("sawtooth down");
  gtk_widget_ref (lfo_waveform_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "lfo_waveform_label", lfo_waveform_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (lfo_waveform_label);
  gtk_box_pack_start (GTK_BOX (hbox3), lfo_waveform_label, FALSE, FALSE, 2);
  gtk_label_set_justify (GTK_LABEL (lfo_waveform_label), GTK_JUSTIFY_LEFT);

    lfo_frequency = create_voice_slider(main_window, "lfo_frequency",
                                       XSYNTH_PORT_LFO_FREQUENCY);
    gtk_table_attach (GTK_TABLE (table7), lfo_frequency, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    lfo_amount_o = create_voice_slider(main_window, "lfo_amount_o",
                                       XSYNTH_PORT_LFO_AMOUNT_O);
    gtk_table_attach (GTK_TABLE (table7), lfo_amount_o, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    lfo_amount_f = create_voice_slider(main_window, "lfo_amount_f",
                                       XSYNTH_PORT_LFO_AMOUNT_F);
    gtk_table_attach (GTK_TABLE (table7), lfo_amount_f, 1, 2, 3, 4,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame5 = gtk_frame_new ("MIXER");
  gtk_widget_ref (frame5);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame5", frame5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame5);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame5, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table8 = gtk_table_new (1, 2, FALSE);
  gtk_widget_ref (table8);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table8", table8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table8);
  gtk_container_add (GTK_CONTAINER (frame5), table8);
  gtk_container_set_border_width (GTK_CONTAINER (table8), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table8), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table8), 5);

  label24 = gtk_label_new ("balance");
  gtk_widget_ref (label24);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label24", label24,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label24);
  gtk_table_attach (GTK_TABLE (table8), label24, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label24), 0, 0.5);

    osc_balance = create_voice_slider(main_window, "osc_balance",
                                      XSYNTH_PORT_OSC_BALANCE);
    gtk_table_attach (GTK_TABLE (table8), osc_balance, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame6 = gtk_frame_new ("PORTAMENTO");
  gtk_widget_ref (frame6);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame6", frame6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame6);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame6, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table9 = gtk_table_new (1, 2, FALSE);
  gtk_widget_ref (table9);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table9", table9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table9);
  gtk_container_add (GTK_CONTAINER (frame6), table9);
  gtk_container_set_border_width (GTK_CONTAINER (table9), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table9), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table9), 5);

  label25 = gtk_label_new ("glide time");
  gtk_widget_ref (label25);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label25", label25,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label25);
  gtk_table_attach (GTK_TABLE (table9), label25, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label25), 0, 0.5);

    glide_time = create_voice_slider(main_window, "glide_time",
                                     XSYNTH_PORT_GLIDE_TIME);
    gtk_table_attach (GTK_TABLE (table9), glide_time, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame7 = gtk_frame_new ("EG1 (VCA)");
  gtk_widget_ref (frame7);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame7", frame7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame7);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame7, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  table10 = gtk_table_new (7, 2, FALSE);
  gtk_widget_ref (table10);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table10", table10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table10);
  gtk_container_add (GTK_CONTAINER (frame7), table10);
  gtk_container_set_border_width (GTK_CONTAINER (table10), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table10), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table10), 5);

  label26 = gtk_label_new ("attack time");
  gtk_widget_ref (label26);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label26", label26,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label26);
  gtk_table_attach (GTK_TABLE (table10), label26, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label26), 0, 0.5);

  label27 = gtk_label_new ("decay time");
  gtk_widget_ref (label27);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label27", label27,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label27);
  gtk_table_attach (GTK_TABLE (table10), label27, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label27), 0, 0.5);

  label28 = gtk_label_new ("release time");
  gtk_widget_ref (label28);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label28", label28,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label28);
  gtk_table_attach (GTK_TABLE (table10), label28, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label28), 0, 0.5);

  label29 = gtk_label_new ("sustain level");
  gtk_widget_ref (label29);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label29", label29,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label29);
  gtk_table_attach (GTK_TABLE (table10), label29, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label29), 0, 0.5);

  label51 = gtk_label_new ("velocity sens");
  gtk_widget_ref (label51);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label51", label51,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label51);
  gtk_table_attach (GTK_TABLE (table10), label51, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label51), 0, 0.5);

  label30 = gtk_label_new ("pitch depth");
  gtk_widget_ref (label30);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label30", label30,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label30);
  gtk_table_attach (GTK_TABLE (table10), label30, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label30), 0, 0.5);

  label31 = gtk_label_new ("filter depth");
  gtk_widget_ref (label31);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label31", label31,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label31);
  gtk_table_attach (GTK_TABLE (table10), label31, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label31), 0, 0.5);

    eg1_attack = create_voice_slider(main_window, "eg1_attack",
                                     XSYNTH_PORT_EG1_ATTACK_TIME);
    gtk_table_attach (GTK_TABLE (table10), eg1_attack, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg1_decay = create_voice_slider(main_window, "eg1_decay",
                                    XSYNTH_PORT_EG1_DECAY_TIME);
    gtk_table_attach (GTK_TABLE (table10), eg1_decay, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg1_sustain = create_voice_slider(main_window, "eg1_sustain",
                                      XSYNTH_PORT_EG1_SUSTAIN_LEVEL);
    gtk_table_attach (GTK_TABLE (table10), eg1_sustain, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg1_release = create_voice_slider(main_window, "eg1_release",
                                      XSYNTH_PORT_EG1_RELEASE_TIME);
    gtk_table_attach (GTK_TABLE (table10), eg1_release, 1, 2, 3, 4,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg1_vel_sens = create_voice_slider(main_window, "eg1_vel_sens",
                                       XSYNTH_PORT_EG1_VEL_SENS);
    gtk_table_attach (GTK_TABLE (table10), eg1_vel_sens, 1, 2, 4, 5,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg1_amount_o = create_voice_slider(main_window, "eg1_amount_o",
                                       XSYNTH_PORT_EG1_AMOUNT_O);
    gtk_table_attach (GTK_TABLE (table10), eg1_amount_o, 1, 2, 5, 6,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg1_amount_f = create_voice_slider(main_window, "eg1_amount_f",
                                       XSYNTH_PORT_EG1_AMOUNT_F);
    gtk_table_attach (GTK_TABLE (table10), eg1_amount_f, 1, 2, 6, 7,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame8 = gtk_frame_new ("EG2");
  gtk_widget_ref (frame8);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame8", frame8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame8);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame8, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table11 = gtk_table_new (7, 2, FALSE);
  gtk_widget_ref (table11);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table11", table11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table11);
  gtk_container_add (GTK_CONTAINER (frame8), table11);
  gtk_container_set_border_width (GTK_CONTAINER (table11), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table11), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table11), 5);

  label32 = gtk_label_new ("attack time");
  gtk_widget_ref (label32);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label32", label32,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label32);
  gtk_table_attach (GTK_TABLE (table11), label32, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label32), 0, 0.5);

  label33 = gtk_label_new ("decay time");
  gtk_widget_ref (label33);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label33", label33,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label33);
  gtk_table_attach (GTK_TABLE (table11), label33, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label33), 0, 0.5);

  label34 = gtk_label_new ("release time");
  gtk_widget_ref (label34);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label34", label34,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label34);
  gtk_table_attach (GTK_TABLE (table11), label34, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label34), 0, 0.5);

  label35 = gtk_label_new ("sustain level");
  gtk_widget_ref (label35);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label35", label35,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label35);
  gtk_table_attach (GTK_TABLE (table11), label35, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label35), 0, 0.5);

  label52 = gtk_label_new ("velocity sens");
  gtk_widget_ref (label52);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label52", label52,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label52);
  gtk_table_attach (GTK_TABLE (table11), label52, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label52), 0, 0.5);

  label36 = gtk_label_new ("pitch depth");
  gtk_widget_ref (label36);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label36", label36,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label36);
  gtk_table_attach (GTK_TABLE (table11), label36, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label36), 0, 0.5);

  label37 = gtk_label_new ("filter depth");
  gtk_widget_ref (label37);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label37", label37,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label37);
  gtk_table_attach (GTK_TABLE (table11), label37, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label37), 0, 0.5);

    eg2_attack = create_voice_slider(main_window, "eg2_attack",
                                     XSYNTH_PORT_EG2_ATTACK_TIME);
    gtk_table_attach (GTK_TABLE (table11), eg2_attack, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg2_decay = create_voice_slider(main_window, "eg2_decay",
                                    XSYNTH_PORT_EG2_DECAY_TIME);
    gtk_table_attach (GTK_TABLE (table11), eg2_decay, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg2_sustain = create_voice_slider(main_window, "eg2_sustain",
                                      XSYNTH_PORT_EG2_SUSTAIN_LEVEL);
    gtk_table_attach (GTK_TABLE (table11), eg2_sustain, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg2_release = create_voice_slider(main_window, "eg2_release",
                                      XSYNTH_PORT_EG2_RELEASE_TIME);
    gtk_table_attach (GTK_TABLE (table11), eg2_release, 1, 2, 3, 4,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg2_vel_sens = create_voice_slider(main_window, "eg2_vel_sens",
                                      XSYNTH_PORT_EG2_VEL_SENS);
    gtk_table_attach (GTK_TABLE (table11), eg2_vel_sens, 1, 2, 4, 5,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg2_amount_o = create_voice_slider(main_window, "eg2_amount_o",
                                       XSYNTH_PORT_EG2_AMOUNT_O);
    gtk_table_attach (GTK_TABLE (table11), eg2_amount_o, 1, 2, 5, 6,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    eg2_amount_f = create_voice_slider(main_window, "eg2_amount_f",
                                       XSYNTH_PORT_EG2_AMOUNT_F);
    gtk_table_attach (GTK_TABLE (table11), eg2_amount_f, 1, 2, 6, 7,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame9 = gtk_frame_new ("VCF");
  gtk_widget_ref (frame9);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame9", frame9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame9);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame9, 0, 1, 4, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  table12 = gtk_table_new (3, 2, FALSE);
  gtk_widget_ref (table12);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table12", table12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table12);
  gtk_container_add (GTK_CONTAINER (frame9), table12);
  gtk_container_set_border_width (GTK_CONTAINER (table12), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table12), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table12), 5);

  label38 = gtk_label_new ("cutoff");
  gtk_widget_ref (label38);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label38", label38,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label38);
  gtk_table_attach (GTK_TABLE (table12), label38, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label38), 0, 0.5);

  label39 = gtk_label_new ("resonance");
  gtk_widget_ref (label39);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label39", label39,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label39);
  gtk_table_attach (GTK_TABLE (table12), label39, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label39), 0, 0.5);

  label40 = gtk_label_new ("12-24 db/oct");
  gtk_widget_ref (label40);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label40", label40,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label40);
  gtk_table_attach (GTK_TABLE (table12), label40, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label40), 0, 0.5);

    vcf_4pole = gtk_check_button_new ();
    voice_widget[XSYNTH_PORT_VCF_4POLE] = (GtkObject *)vcf_4pole;
  gtk_widget_ref (vcf_4pole);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "vcf_4pole", vcf_4pole,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vcf_4pole);
  gtk_table_attach (GTK_TABLE (table12), vcf_4pole, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

    vcf_cutoff = create_voice_slider(main_window, "vcf_cutoff",
                                     XSYNTH_PORT_VCF_CUTOFF);
    gtk_table_attach (GTK_TABLE (table12), vcf_cutoff, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    vcf_qres = create_voice_slider(main_window, "vcf_qres",
                                   XSYNTH_PORT_VCF_QRES);
    gtk_table_attach (GTK_TABLE (table12), vcf_qres, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame10 = gtk_frame_new ("VOLUME");
  gtk_widget_ref (frame10);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame10", frame10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame10);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame10, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table13 = gtk_table_new (1, 2, FALSE);
  gtk_widget_ref (table13);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table13", table13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table13);
  gtk_container_add (GTK_CONTAINER (frame10), table13);
  gtk_container_set_border_width (GTK_CONTAINER (table13), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table13), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table13), 5);

  label41 = gtk_label_new ("volume");
  gtk_widget_ref (label41);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label41", label41,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label41);
  gtk_table_attach (GTK_TABLE (table13), label41, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label41), 0, 0.5);

    volume = create_voice_slider(main_window, "volume",
                                 XSYNTH_PORT_VOLUME);
    gtk_table_attach (GTK_TABLE (table13), volume, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame11 = gtk_frame_new ("NAME");
  gtk_widget_ref (frame11);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame11", frame11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame11);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame11, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table14 = gtk_table_new (1, 2, FALSE);
  gtk_widget_ref (table14);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table14", table14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table14);
  gtk_container_add (GTK_CONTAINER (frame11), table14);
  gtk_container_set_border_width (GTK_CONTAINER (table14), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table14), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table14), 5);

  label42 = gtk_label_new ("name");
  gtk_widget_ref (label42);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label42", label42,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label42);
  gtk_table_attach (GTK_TABLE (table14), label42, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label42), 0, 0.5);

  name_entry = gtk_entry_new_with_max_length(30);
  gtk_widget_ref (name_entry);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "name_entry", name_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (name_entry);
  gtk_table_attach (GTK_TABLE (table14), name_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  frame12 = gtk_frame_new ("TEST NOTE");
  gtk_widget_ref (frame12);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame12", frame12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame12);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame12, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  table15 = gtk_table_new (3, 2, FALSE);
  gtk_widget_ref (table15);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table15", table15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table15);
  gtk_container_add (GTK_CONTAINER (frame12), table15);
  gtk_container_set_border_width (GTK_CONTAINER (table15), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table15), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table15), 5);

  label10 = gtk_label_new ("key");
  gtk_widget_ref (label10);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label10", label10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label10);
  gtk_table_attach (GTK_TABLE (table15), label10, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_padding (GTK_MISC (label10), 9, 0);

  label53 = gtk_label_new ("velocity");
  gtk_widget_ref (label53);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label53", label53,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label53);
  gtk_table_attach (GTK_TABLE (table15), label53, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label53), 0, 0.5);

  test_note_button = gtk_button_new_with_label ("Send Test Note");
  gtk_widget_ref (test_note_button);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_button", test_note_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (test_note_button);
  gtk_table_attach (GTK_TABLE (table15), test_note_button, 0, 2, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 4, 0);

  test_note_key = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (60, 12, 120, 1, 12, 12)));
  gtk_widget_ref (test_note_key);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_key", test_note_key,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (test_note_key);
  gtk_table_attach (GTK_TABLE (table15), test_note_key, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (test_note_key), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (test_note_key), 0);
  gtk_range_set_update_policy (GTK_RANGE (test_note_key), GTK_UPDATE_DELAYED);

  test_note_velocity = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (96, 1, 137, 1, 10, 10)));
  gtk_widget_ref (test_note_velocity);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_velocity", test_note_velocity,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (test_note_velocity);
  gtk_table_attach (GTK_TABLE (table15), test_note_velocity, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (test_note_velocity), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (test_note_velocity), 0);
  gtk_range_set_update_policy (GTK_RANGE (test_note_velocity), GTK_UPDATE_DELAYED);

  frame13 = gtk_frame_new ("EDIT ACTION");
  gtk_widget_ref (frame13);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame13", frame13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame13);
  gtk_table_attach (GTK_TABLE (patch_edit_table), frame13, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table17 = gtk_table_new (2, 1, FALSE);
  gtk_widget_ref (table17);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table17", table17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table17);
  gtk_container_add (GTK_CONTAINER (frame13), table17);
  gtk_container_set_border_width (GTK_CONTAINER (table17), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table17), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table17), 5);

  save_changes = gtk_button_new_with_label ("Save Changes");
  gtk_widget_ref (save_changes);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "save_changes", save_changes,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (save_changes);
  gtk_table_attach (GTK_TABLE (table17), save_changes, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

  discard_changes = gtk_button_new_with_label ("Discard Changes");
  gtk_widget_ref (discard_changes);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "discard_changes", discard_changes,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (discard_changes);
  gtk_table_attach (GTK_TABLE (table17), discard_changes, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

  patch_edit_tab_label = gtk_label_new ("Patch Edit");
  gtk_widget_ref (patch_edit_tab_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "patch_edit_tab_label", patch_edit_tab_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (patch_edit_tab_label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), patch_edit_tab_label);

    /* Configuration tab */
  frame14 = gtk_frame_new ("Synthesizer");
  gtk_widget_ref (frame14);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame14", frame14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame14);
  gtk_container_add (GTK_CONTAINER (notebook1), frame14);

  table16 = gtk_table_new (4, 3, FALSE);
  gtk_widget_ref (table16);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table16", table16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table16);
  gtk_container_add (GTK_CONTAINER (frame14), table16);
  gtk_container_set_border_width (GTK_CONTAINER (table16), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table16), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table16), 5);

  label54 = gtk_label_new ("Tuning");
  gtk_widget_ref (label54);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label54", label54,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label54);
  gtk_table_attach (GTK_TABLE (table16), label54, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label54), 0, 0.5);

  tuning_spin_adj = gtk_adjustment_new (440, 415.3, 467.2, 0.1, 1, 1);
  tuning_spin = gtk_spin_button_new (GTK_ADJUSTMENT (tuning_spin_adj), 1, 1);
  gtk_widget_ref (tuning_spin);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "tuning_spin", tuning_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (tuning_spin);
  gtk_table_attach (GTK_TABLE (table16), tuning_spin, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (tuning_spin), TRUE);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (tuning_spin), GTK_UPDATE_IF_VALID);

    monophonic_option_menu = gtk_option_menu_new ();
    gtk_widget_ref (monophonic_option_menu);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "monophonic_option_menu", monophonic_option_menu,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (monophonic_option_menu);
    gtk_table_attach (GTK_TABLE (table16), monophonic_option_menu, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    optionmenu5_menu = gtk_menu_new ();
    mono_mode_off = gtk_menu_item_new_with_label ("Off");
    gtk_widget_show (mono_mode_off);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_off);
    mono_mode_on = gtk_menu_item_new_with_label ("On");
    gtk_widget_show (mono_mode_on);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_on);
    mono_mode_once = gtk_menu_item_new_with_label ("Once");
    gtk_widget_show (mono_mode_once);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_once);
    mono_mode_both = gtk_menu_item_new_with_label ("Both");
    gtk_widget_show (mono_mode_both);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_both);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (monophonic_option_menu), optionmenu5_menu);

  polyphony_adj = gtk_adjustment_new (XSYNTH_DEFAULT_POLYPHONY, 1, 128, 1, 10, 10);
  polyphony = gtk_spin_button_new (GTK_ADJUSTMENT (polyphony_adj), 1, 0);
  gtk_widget_ref (polyphony);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "polyphony", polyphony,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (polyphony);
  gtk_table_attach (GTK_TABLE (table16), polyphony, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label43 = gtk_label_new ("Polyphony");
  gtk_widget_ref (label43);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label43", label43,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label43);
  gtk_table_attach (GTK_TABLE (table16), label43, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label43), 0, 0.5);

  label44 = gtk_label_new ("Monophonic Mode");
  gtk_widget_ref (label44);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label44", label44,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label44);
  gtk_table_attach (GTK_TABLE (table16), label44, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label44), 0, 0.5);

  frame15 = gtk_frame_new (NULL);
  gtk_widget_ref (frame15);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame15", frame15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame15);
  gtk_table_attach (GTK_TABLE (table16), frame15, 0, 3, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);

  logo_pixmap = create_logo_pixmap (main_window);
  gtk_widget_ref (logo_pixmap);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "logo_pixmap", logo_pixmap,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (logo_pixmap);
  gtk_container_add (GTK_CONTAINER (frame15), logo_pixmap);
  gtk_misc_set_padding (GTK_MISC (logo_pixmap), 2, 2);

  label47 = gtk_label_new ("     ");
  gtk_widget_ref (label47);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label47", label47,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label47);
  gtk_table_attach (GTK_TABLE (table16), label47, 2, 3, 0, 3,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label47), 0, 0.5);

  configuration_tab_label = gtk_label_new ("Configuration");
  gtk_widget_ref (configuration_tab_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "configuration_tab_label", configuration_tab_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (configuration_tab_label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 2), configuration_tab_label);

  statusbar1 = gtk_statusbar_new ();
  gtk_widget_ref (statusbar1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "statusbar1", statusbar1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (statusbar1);
  gtk_box_pack_start (GTK_BOX (vbox1), statusbar1, FALSE, FALSE, 0);

    gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
                       GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_menu_quit_activate);
    
  gtk_signal_connect (GTK_OBJECT (menu_open), "activate",
                      GTK_SIGNAL_FUNC (on_menu_open_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (menu_save), "activate",
                      GTK_SIGNAL_FUNC (on_menu_save_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (menu_quit), "activate",
                      GTK_SIGNAL_FUNC (on_menu_quit_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (menu_about), "activate",
                      GTK_SIGNAL_FUNC (on_menu_about_activate),
                      NULL);
    gtk_signal_connect(GTK_OBJECT(notebook1), "switch_page",
                       GTK_SIGNAL_FUNC(on_notebook_switch_page),
                       NULL);
    gtk_signal_connect(GTK_OBJECT(patches_clist), "select_row",
                       GTK_SIGNAL_FUNC(on_patches_selection),
                       NULL);

    /* connect patch edit widgets */
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(osc1_pitch)),
                       "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                       (gpointer)XSYNTH_PORT_OSC1_PITCH);
    gtk_signal_connect (GTK_OBJECT (osc1_waveform_adj), "value_changed",
                        GTK_SIGNAL_FUNC (on_voice_detent_change),
                        (gpointer)XSYNTH_PORT_OSC1_WAVEFORM);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(osc1_pulsewidth)),
                        "value_changed", GTK_SIGNAL_FUNC (on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_OSC1_PULSEWIDTH);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(osc2_pitch)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_OSC2_PITCH);
    gtk_signal_connect (GTK_OBJECT (osc2_waveform_adj), "value_changed",
                        GTK_SIGNAL_FUNC (on_voice_detent_change),
                        (gpointer)XSYNTH_PORT_OSC2_WAVEFORM);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(osc2_pulsewidth)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_OSC2_PULSEWIDTH);
    gtk_signal_connect (GTK_OBJECT (osc_sync), "toggled",
                        GTK_SIGNAL_FUNC (on_voice_onoff_toggled),
                        (gpointer)XSYNTH_PORT_OSC_SYNC);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(osc_balance)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_OSC_BALANCE);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(lfo_frequency)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_LFO_FREQUENCY);
    gtk_signal_connect (GTK_OBJECT (lfo_waveform_adj), "value_changed",
                        GTK_SIGNAL_FUNC (on_voice_detent_change),
                        (gpointer)XSYNTH_PORT_LFO_WAVEFORM);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(lfo_amount_o)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_LFO_AMOUNT_O);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(lfo_amount_f)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_LFO_AMOUNT_F);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg1_attack)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG1_ATTACK_TIME);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg1_decay)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG1_DECAY_TIME);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg1_sustain)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG1_SUSTAIN_LEVEL);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg1_release)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG1_RELEASE_TIME);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg1_vel_sens)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG1_VEL_SENS);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg1_amount_o)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG1_AMOUNT_O);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg1_amount_f)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG1_AMOUNT_F);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg2_attack)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG2_ATTACK_TIME);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg2_decay)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG2_DECAY_TIME);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg2_sustain)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG2_SUSTAIN_LEVEL);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg2_release)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG2_RELEASE_TIME);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg2_vel_sens)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG2_VEL_SENS);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg2_amount_o)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG2_AMOUNT_O);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(eg2_amount_f)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_EG2_AMOUNT_F);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(vcf_cutoff)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_VCF_CUTOFF);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(vcf_qres)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_VCF_QRES);
    gtk_signal_connect (GTK_OBJECT (vcf_4pole), "toggled",
                        GTK_SIGNAL_FUNC (on_voice_onoff_toggled),
                        (gpointer)XSYNTH_PORT_VCF_4POLE);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(glide_time)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_GLIDE_TIME);
    gtk_signal_connect (GTK_OBJECT (GET_VOICE_SLIDER_ADJUSTMENT(volume)),
                        "value_changed", GTK_SIGNAL_FUNC(on_voice_slider_change),
                        (gpointer)XSYNTH_PORT_VOLUME);

    /* connect test note widgets */
    gtk_signal_connect (GTK_OBJECT (gtk_range_get_adjustment (GTK_RANGE (test_note_key))),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        (gpointer)0);
    gtk_signal_connect (GTK_OBJECT (gtk_range_get_adjustment (GTK_RANGE (test_note_velocity))),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (test_note_button), "pressed",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (test_note_button), "released",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        (gpointer)0);

    /* connect edit action buttons */
    gtk_signal_connect (GTK_OBJECT (discard_changes), "clicked",
                        GTK_SIGNAL_FUNC (on_edit_action_button_press),
                        (gpointer)0);
    gtk_widget_set_sensitive (discard_changes, FALSE); // !FIX! remove when implemented
    gtk_signal_connect (GTK_OBJECT (save_changes), "clicked",
                        GTK_SIGNAL_FUNC (on_edit_action_button_press),
                        (gpointer)1);

    /* connect synth configuration widgets */
    gtk_signal_connect (GTK_OBJECT (tuning_spin_adj), "value_changed",
                        GTK_SIGNAL_FUNC(on_tuning_change),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (polyphony))),
                        "value_changed", GTK_SIGNAL_FUNC(on_polyphony_change),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (mono_mode_off), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"off");
    gtk_signal_connect (GTK_OBJECT (mono_mode_on), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"on");
    gtk_signal_connect (GTK_OBJECT (mono_mode_once), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"once");
    gtk_signal_connect (GTK_OBJECT (mono_mode_both), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"both");

    gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
}

void
create_about_window (const char *tag)
{
    GtkWidget *frame1;
    GtkWidget *vbox2;
    GtkWidget *about_pixmap;
    GtkWidget *closeabout;

    about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (about_window), "about_window", about_window);
    gtk_window_set_title (GTK_WINDOW (about_window), "About Xsynth-DSSI");
    gtk_widget_realize(about_window);  /* window must be realized for create_about_pixmap() */

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox2);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "vbox2", vbox2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (about_window), vbox2);

    frame1 = gtk_frame_new (NULL);
    gtk_widget_ref (frame1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "frame1", frame1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (frame1);
    gtk_box_pack_start (GTK_BOX (vbox2), frame1, FALSE, FALSE, 0);

    about_pixmap = (GtkWidget *)create_about_pixmap (about_window);
    gtk_widget_ref (about_pixmap);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "about_pixmap", about_pixmap,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (about_pixmap);
    gtk_container_add (GTK_CONTAINER (frame1), about_pixmap);
    gtk_misc_set_padding (GTK_MISC (about_pixmap), 5, 5);

    about_label = gtk_label_new ("Some message\ngoes here");
    gtk_widget_ref (about_label);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "about_label", about_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (about_label);
    gtk_box_pack_start (GTK_BOX (vbox2), about_label, FALSE, FALSE, 0);
    // gtk_label_set_line_wrap (GTK_LABEL (about_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (about_label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_padding (GTK_MISC (about_label), 5, 5);

    closeabout = gtk_button_new_with_label ("Dismiss");
    gtk_widget_ref (closeabout);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "closeabout", closeabout,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (closeabout);
    gtk_box_pack_start (GTK_BOX (vbox2), closeabout, FALSE, FALSE, 0);

    gtk_signal_connect (GTK_OBJECT (about_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (about_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_about_dismiss);
    gtk_signal_connect (GTK_OBJECT (closeabout), "clicked",
                        GTK_SIGNAL_FUNC (on_about_dismiss),
                        NULL);
}

void
create_open_file_selection (const char *tag)
{
  char      *title;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;

    title = (char *)malloc(strlen(tag) + 19);
    sprintf(title, "%s - Load Patch Bank", tag);
    open_file_selection = gtk_file_selection_new (title);
    free(title);
  gtk_object_set_data (GTK_OBJECT (open_file_selection), "open_file_selection", open_file_selection);
  gtk_container_set_border_width (GTK_CONTAINER (open_file_selection), 10);
  GTK_WINDOW (open_file_selection)->type = GTK_WINDOW_TOPLEVEL;

  ok_button = GTK_FILE_SELECTION (open_file_selection)->ok_button;
  gtk_object_set_data (GTK_OBJECT (open_file_selection), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);

  cancel_button = GTK_FILE_SELECTION (open_file_selection)->cancel_button;
  gtk_object_set_data (GTK_OBJECT (open_file_selection), "cancel_button", cancel_button);
  gtk_widget_show (cancel_button);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

    gtk_signal_connect (GTK_OBJECT (open_file_selection), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (open_file_selection), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_open_file_cancel);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (open_file_selection)->ok_button),
                        "clicked", (GtkSignalFunc)on_open_file_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (open_file_selection)->cancel_button),
                        "clicked", (GtkSignalFunc)on_open_file_cancel,
                        NULL);
}

void
create_save_file_selection (const char *tag)
{
  char      *title;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;

    title = (char *)malloc(strlen(tag) + 19);
    sprintf(title, "%s - Save Patch Bank", tag);
    save_file_selection = gtk_file_selection_new (title);
    free(title);
  gtk_object_set_data (GTK_OBJECT (save_file_selection), "save_file_selection", save_file_selection);
  gtk_container_set_border_width (GTK_CONTAINER (save_file_selection), 10);
  GTK_WINDOW (save_file_selection)->type = GTK_WINDOW_TOPLEVEL;

  ok_button = GTK_FILE_SELECTION (save_file_selection)->ok_button;
  gtk_object_set_data (GTK_OBJECT (save_file_selection), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);

  cancel_button = GTK_FILE_SELECTION (save_file_selection)->cancel_button;
  gtk_object_set_data (GTK_OBJECT (save_file_selection), "cancel_button", cancel_button);
  gtk_widget_show (cancel_button);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

    gtk_signal_connect (GTK_OBJECT (save_file_selection), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (save_file_selection), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_save_file_cancel);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (save_file_selection)->ok_button),
                        "clicked", (GtkSignalFunc)on_save_file_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (save_file_selection)->cancel_button),
                        "clicked", (GtkSignalFunc)on_save_file_cancel,
                        NULL);
}

void
create_notice_window (const char *tag)
{
  char      *title;
  GtkWidget *vbox3;
  GtkWidget *hbox1;
  GtkWidget *notice_dismiss;

    notice_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (notice_window), "notice_window", notice_window);
    title = (char *)malloc(strlen(tag) + 8);
    sprintf(title, "%s Notice", tag);
    gtk_window_set_title (GTK_WINDOW (notice_window), title);
    free(title);
    gtk_window_set_position (GTK_WINDOW (notice_window), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal (GTK_WINDOW (notice_window), TRUE);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (notice_window), vbox3);

  notice_label_1 = gtk_label_new ("Some message\ngoes here");
  gtk_widget_ref (notice_label_1);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_1", notice_label_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_label_1);
  gtk_box_pack_start (GTK_BOX (vbox3), notice_label_1, TRUE, TRUE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (notice_label_1), TRUE);
  gtk_misc_set_padding (GTK_MISC (notice_label_1), 10, 5);

  notice_label_2 = gtk_label_new ("more text\ngoes here");
  gtk_widget_ref (notice_label_2);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_2", notice_label_2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_label_2);
  gtk_box_pack_start (GTK_BOX (vbox3), notice_label_2, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (notice_label_2), TRUE);
  gtk_misc_set_padding (GTK_MISC (notice_label_2), 10, 5);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox1, FALSE, FALSE, 0);

  notice_dismiss = gtk_button_new_with_label ("Dismiss");
  gtk_widget_ref (notice_dismiss);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_dismiss", notice_dismiss,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_dismiss);
  gtk_box_pack_start (GTK_BOX (hbox1), notice_dismiss, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (notice_dismiss), 7);

    gtk_signal_connect (GTK_OBJECT (notice_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (notice_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_notice_dismiss);
    gtk_signal_connect (GTK_OBJECT (notice_dismiss), "clicked",
                        GTK_SIGNAL_FUNC (on_notice_dismiss),
                        NULL);
}

void
create_open_file_position_window (const char *tag)
{
  char      *title;
  GtkWidget *vbox4;
  GtkWidget *position_text_label;
  GtkWidget *hbox2;
  GtkWidget *label50;
  GtkWidget *position_spin;
  GtkWidget *hbox3;
  GtkWidget *position_cancel;
  GtkWidget *position_ok;

    open_file_position_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (open_file_position_window),
                       "open_file_position_window", open_file_position_window);
    title = (char *)malloc(strlen(tag) + 15);
    sprintf(title, "%s Load Position", tag);
    gtk_window_set_title (GTK_WINDOW (open_file_position_window), title);
    free(title);

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox4);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window), "vbox4",
                            vbox4, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (open_file_position_window), vbox4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox4), 6);

  position_text_label = gtk_label_new ("Select the Program Number at which you "
                                       "wish to begin loading patches (existing "
                                       "patches will be overwritten)");
  gtk_widget_ref (position_text_label);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window),
                            "position_text_label", position_text_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_text_label);
  gtk_box_pack_start (GTK_BOX (vbox4), position_text_label, TRUE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (position_text_label), GTK_JUSTIFY_FILL);
  gtk_label_set_line_wrap (GTK_LABEL (position_text_label), TRUE);
  gtk_misc_set_padding (GTK_MISC (position_text_label), 0, 6);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window), "hbox2",
                            hbox2, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox2, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2), 6);

  label50 = gtk_label_new ("Program Number");
  gtk_widget_ref (label50);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window), "label50",
                            label50, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label50);
  gtk_box_pack_start (GTK_BOX (hbox2), label50, FALSE, TRUE, 2);

  open_file_position_spin_adj = gtk_adjustment_new (0, 0, 127, 1, 10, 10);
  position_spin = gtk_spin_button_new (GTK_ADJUSTMENT (open_file_position_spin_adj), 1, 0);
  gtk_widget_ref (position_spin);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window),
                            "position_spin", position_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_spin);
  gtk_box_pack_start (GTK_BOX (hbox2), position_spin, FALSE, FALSE, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (position_spin), TRUE);

  open_file_position_name_label = gtk_label_new ("default voice");
  gtk_widget_ref (open_file_position_name_label);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window),
                            "open_file_position_name_label",
                            open_file_position_name_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (open_file_position_name_label);
  gtk_box_pack_start (GTK_BOX (hbox2), open_file_position_name_label, FALSE, FALSE, 2);
  gtk_label_set_justify (GTK_LABEL (open_file_position_name_label), GTK_JUSTIFY_LEFT);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox3);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window), "hbox3", hbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox3, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox3), 6);

  position_cancel = gtk_button_new_with_label ("Cancel");
  gtk_widget_ref (position_cancel);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window),
                            "position_cancel", position_cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_cancel);
  gtk_box_pack_start (GTK_BOX (hbox3), position_cancel, TRUE, FALSE, 12);

  position_ok = gtk_button_new_with_label ("Load");
  gtk_widget_ref (position_ok);
  gtk_object_set_data_full (GTK_OBJECT (open_file_position_window),
                            "position_ok", position_ok,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_ok);
  gtk_box_pack_end (GTK_BOX (hbox3), position_ok, TRUE, FALSE, 12);

    gtk_signal_connect (GTK_OBJECT (open_file_position_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (open_file_position_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_open_file_position_cancel);
    gtk_signal_connect (GTK_OBJECT (open_file_position_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_position_change),
                        (gpointer)open_file_position_name_label);
    gtk_signal_connect (GTK_OBJECT (position_ok), "clicked",
                        (GtkSignalFunc)on_open_file_position_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (position_cancel), "clicked",
                        (GtkSignalFunc)on_open_file_position_cancel,
                        NULL);
}

void
create_edit_save_position_window (const char *tag)
{
  char      *title;
  GtkWidget *vbox4;
  GtkWidget *position_text_label;
  GtkWidget *hbox2;
  GtkWidget *label50;
  GtkWidget *position_spin;
  GtkWidget *hbox3;
  GtkWidget *position_cancel;
  GtkWidget *position_ok;

    edit_save_position_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (edit_save_position_window),
                       "edit_save_position_window", edit_save_position_window);
    title = (char *)malloc(strlen(tag) + 20);
    sprintf(title, "%s Edit Save Position", tag);
    gtk_window_set_title (GTK_WINDOW (edit_save_position_window), title);
    free(title);

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox4);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "vbox4",
                            vbox4, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (edit_save_position_window), vbox4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox4), 6);

  position_text_label = gtk_label_new ("Select the Program Number to which you "
                                       "wish to save your changed patch "
                                       "(existing patches will be overwritten):");
  gtk_widget_ref (position_text_label);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_text_label", position_text_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_text_label);
  gtk_box_pack_start (GTK_BOX (vbox4), position_text_label, TRUE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (position_text_label), GTK_JUSTIFY_FILL);
  gtk_label_set_line_wrap (GTK_LABEL (position_text_label), TRUE);
  gtk_misc_set_padding (GTK_MISC (position_text_label), 0, 6);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "hbox2",
                            hbox2, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox2, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2), 6);

  label50 = gtk_label_new ("Program Number");
  gtk_widget_ref (label50);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "label50",
                            label50, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label50);
  gtk_box_pack_start (GTK_BOX (hbox2), label50, FALSE, TRUE, 2);

  edit_save_position_spin_adj = gtk_adjustment_new (0, 0, 127, 1, 10, 10);
  position_spin = gtk_spin_button_new (GTK_ADJUSTMENT (edit_save_position_spin_adj), 1, 0);
  gtk_widget_ref (position_spin);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_spin", position_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_spin);
  gtk_box_pack_start (GTK_BOX (hbox2), position_spin, FALSE, FALSE, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (position_spin), TRUE);

  edit_save_position_name_label = gtk_label_new ("default voice");
  gtk_widget_ref (edit_save_position_name_label);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "edit_save_position_name_label",
                            edit_save_position_name_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (edit_save_position_name_label);
  gtk_box_pack_start (GTK_BOX (hbox2), edit_save_position_name_label, FALSE, FALSE, 2);
  gtk_label_set_justify (GTK_LABEL (edit_save_position_name_label), GTK_JUSTIFY_LEFT);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox3);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "hbox3", hbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox3, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox3), 6);

  position_cancel = gtk_button_new_with_label ("Cancel");
  gtk_widget_ref (position_cancel);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_cancel", position_cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_cancel);
  gtk_box_pack_start (GTK_BOX (hbox3), position_cancel, TRUE, FALSE, 12);

  position_ok = gtk_button_new_with_label ("Save");
  gtk_widget_ref (position_ok);
  gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                            "position_ok", position_ok,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (position_ok);
  gtk_box_pack_end (GTK_BOX (hbox3), position_ok, TRUE, FALSE, 12);

    gtk_signal_connect (GTK_OBJECT (edit_save_position_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (edit_save_position_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_edit_save_position_cancel);
    gtk_signal_connect (GTK_OBJECT (edit_save_position_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_position_change),
                        (gpointer)edit_save_position_name_label);
    gtk_signal_connect (GTK_OBJECT (position_ok), "clicked",
                        (GtkSignalFunc)on_edit_save_position_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (position_cancel), "clicked",
                        (GtkSignalFunc)on_edit_save_position_cancel,
                        NULL);
}

void
create_windows(const char *instance_tag)
{
    char tag[50];

    /* build a nice identifier string for the window titles */
    if (strlen(instance_tag) == 0) {
        strcpy(tag, "Xsynth-DSSI");
    } else if (strstr(instance_tag, "Xsynth-DSSI") ||
               strstr(instance_tag, "xsynth-dssi")) {
        if (strlen(instance_tag) > 49) {
            snprintf(tag, 50, "...%s", instance_tag + strlen(instance_tag) - 46); /* hope the unique info is at the end */
        } else {
            strcpy(tag, instance_tag);
        }
    } else {
        if (strlen(instance_tag) > 37) {
            snprintf(tag, 50, "Xsynth-DSSI ...%s", instance_tag + strlen(instance_tag) - 34);
        } else {
            snprintf(tag, 50, "Xsynth-DSSI %s", instance_tag);
        }
    }

    create_main_window(tag);
    create_about_window(tag);
    create_open_file_selection(tag);
    create_save_file_selection(tag);
    create_open_file_position_window(tag);
    create_edit_save_position_window(tag);
    create_notice_window(tag);
}

