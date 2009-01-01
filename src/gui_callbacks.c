/* Xsynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include <lo/lo.h>

#include "xsynth_types.h"
#include "xsynth.h"
#include "xsynth_ports.h"
#include "xsynth_voice.h"
#include "gui_main.h"
#include "gui_callbacks.h"
#include "gui_images.h"
#include "gui_interface.h"
#include "gui_data.h"

extern GtkObject *voice_widget[XSYNTH_PORTS_COUNT];

extern xsynth_patch_t *patches;

extern lo_address osc_host_address;
extern char *     osc_configure_path;
extern char *     osc_control_path;
extern char *     osc_midi_path;
extern char *     osc_program_path;
extern char *     osc_update_path;

static int internal_gui_update_only = 0;

static unsigned char test_note_noteon_key = 60;
static unsigned char test_note_noteoff_key;
static unsigned char test_note_velocity = 96;

static gchar *file_selection_last_filename = NULL;
extern char  *project_directory;

static int save_file_start;
static int save_file_end;

#if !GTK_CHECK_VERSION(2, 0, 0)
static int vcf_mode = 0;
#endif

void
file_selection_set_path(GtkWidget *file_selection)
{
    if (file_selection_last_filename) {
        gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection),
                                        file_selection_last_filename);
    } else if (project_directory && strlen(project_directory)) {
        if (project_directory[strlen(project_directory) - 1] != '/') {
            char buffer[PATH_MAX];
            snprintf(buffer, PATH_MAX, "%s/", project_directory);
            gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection),
                                            buffer);
        } else {
            gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection),
                                            project_directory);
        }
    }
}

void
on_menu_open_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_widget_hide(save_file_selection);
    gtk_widget_hide(open_file_position_window);
    gtk_widget_hide(save_file_range_window);
    file_selection_set_path(open_file_selection);
    gtk_widget_show(open_file_selection);
}


void
on_menu_save_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_widget_hide(open_file_selection);
    gtk_widget_hide(open_file_position_window);
    gtk_widget_hide(save_file_range_window);
    file_selection_set_path(save_file_selection);
    gtk_widget_show(save_file_selection);
}


void
on_menu_quit_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    // -FIX- if any patches changed, ask "are you sure?" or "save changes?"
    gtk_main_quit();
}


void
on_menu_about_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char buf[256];
    snprintf(buf, 256, "Xsynth-DSSI version: " VERSION "\n"
                       "plugin URL: %s\n"
                       "host URL: %s\n", osc_self_url, osc_host_url);
    gtk_label_set_text (GTK_LABEL (about_label), buf);
    gtk_widget_show(about_window);
}

gint
on_delete_event_wrapper( GtkWidget *widget, GdkEvent *event, gpointer data )
{
    void (*handler)(GtkWidget *, gpointer) = (void (*)(GtkWidget *, gpointer))data;

    /* call our 'close', 'dismiss' or 'cancel' callback (which must not need the user data) */
    (*handler)(widget, NULL);

    /* tell GTK+ to NOT emit 'destroy' */
    return TRUE;
}

void
on_open_file_ok( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(open_file_selection);
    file_selection_last_filename = (gchar *)gtk_file_selection_get_filename(
                                                GTK_FILE_SELECTION(open_file_selection));

    GDB_MESSAGE(GDB_GUI, " on_open_file_ok: file '%s' selected\n",
                    file_selection_last_filename);

    /* update patch name */
    gtk_signal_emit_by_name (GTK_OBJECT (open_file_position_spin_adj), "value_changed");

    gtk_widget_show(open_file_position_window);
}

void
on_open_file_cancel( GtkWidget *widget, gpointer data )
{
    GDB_MESSAGE(GDB_GUI, ": on_open_file_cancel called\n");
    gtk_widget_hide(open_file_selection);
}

/*
 * on_position_change
 *
 * used by both the open file position and edit save position dialogs
 * data is a pointer to the dialog's patch name label
 */
void
on_position_change(GtkWidget *widget, gpointer data)
{
    int position = lrintf(GTK_ADJUSTMENT(widget)->value);
    GtkWidget *label = (GtkWidget *)data;

    gtk_label_set_text (GTK_LABEL (label), patches[position].name);
}

void
on_open_file_position_ok( GtkWidget *widget, gpointer data )
{
    int position = lrintf(GTK_ADJUSTMENT(open_file_position_spin_adj)->value);
    char *message;

    gtk_widget_hide(open_file_position_window);

    GDB_MESSAGE(GDB_GUI, " on_open_file_position_ok: position %d\n", position);

    if (gui_data_load(file_selection_last_filename, position, &message)) {

        /* successfully loaded at least one patch */
        rebuild_patches_clist();
        display_notice("Load Patch File succeeded:", message);
        gui_data_send_dirty_patch_sections();

    } else {  /* didn't load anything successfully */

        display_notice("Load Patch File failed:", message);

    }
    free(message);
}

void
on_open_file_position_cancel( GtkWidget *widget, gpointer data )
{
    GDB_MESSAGE(GDB_GUI, " on_open_file_position_cancel called\n");
    gtk_widget_hide(open_file_position_window);
}

void
on_save_file_ok( GtkWidget *widget, gpointer data )
{
    int i;

    gtk_widget_hide(save_file_selection);
    file_selection_last_filename = (gchar *)gtk_file_selection_get_filename(
                                                GTK_FILE_SELECTION(save_file_selection));

    GDB_MESSAGE(GDB_GUI, " on_save_file_ok: file '%s' selected\n",
                    file_selection_last_filename);

    /* find the last non-init-voice patch, and set the end position to it, then
     * update the patch names */
    for (i = 127;
         i > 0 && gui_data_patch_compare(&patches[i], &xsynth_init_voice);
         i--);
    (GTK_ADJUSTMENT(save_file_end_spin_adj))->value = (float)i;
    gtk_signal_emit_by_name (GTK_OBJECT (save_file_start_spin_adj), "value_changed");
    gtk_signal_emit_by_name (GTK_OBJECT (save_file_end_spin_adj), "value_changed");

    gtk_widget_show(save_file_range_window);
}

void
on_save_file_cancel( GtkWidget *widget, gpointer data )
{
    GDB_MESSAGE(GDB_GUI, ": on_save_file_cancel called\n");
    gtk_widget_hide(save_file_selection);
}

/*
 * on_save_file_range_change
 */
void
on_save_file_range_change(GtkWidget *widget, gpointer data)
{
    int which = (int)data;
    int start = lrintf(GTK_ADJUSTMENT(save_file_start_spin_adj)->value);
    int end   = lrintf(GTK_ADJUSTMENT(save_file_end_spin_adj)->value);

    if (which == 0) {  /* start */
        if (end < start) {
            (GTK_ADJUSTMENT(save_file_end_spin_adj))->value = (float)start;
            gtk_signal_emit_by_name (GTK_OBJECT (save_file_end_spin_adj), "value_changed");
        }
        gtk_label_set_text (GTK_LABEL (save_file_start_name), patches[start].name);
    } else { /* end */
        if (end < start) {
            (GTK_ADJUSTMENT(save_file_start_spin_adj))->value = (float)end;
            gtk_signal_emit_by_name (GTK_OBJECT (save_file_start_spin_adj), "value_changed");
        }
        gtk_label_set_text (GTK_LABEL (save_file_end_name), patches[end].name);
    }
}

void
on_save_file_range_ok( GtkWidget *widget, gpointer data )
{
    char *message;

    save_file_start = lrintf(GTK_ADJUSTMENT(save_file_start_spin_adj)->value);
    save_file_end   = lrintf(GTK_ADJUSTMENT(save_file_end_spin_adj)->value);

    GDB_MESSAGE(GDB_GUI, " on_save_file_range_ok: start %d, end %d\n",
                save_file_start, save_file_end);

    gtk_widget_hide(save_file_range_window);

    if (gui_data_save(file_selection_last_filename, save_file_start,
                      save_file_end, &message)) {

        display_notice("Save Patch File succeeded:", message);

    } else {  /* problem with save */

        display_notice("Save Patch File failed:", message);

    }
    free(message);
}

void
on_save_file_range_cancel( GtkWidget *widget, gpointer data )
{
    GDB_MESSAGE(GDB_GUI, " on_save_file_range_cancel called\n");
    gtk_widget_hide(save_file_range_window);
}

void
on_about_dismiss( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(about_window);
}

void
on_notebook_switch_page(GtkNotebook     *notebook,
                        GtkNotebookPage *page,
                        guint            page_num)
{
    GDB_MESSAGE(GDB_GUI, " on_notebook_switch_page: page %d selected\n", page_num);
}

void
on_patches_selection(GtkWidget      *clist,
                     gint            row,
                     gint            column,
                     GdkEventButton *event,
                     gpointer        data )
{
    if (internal_gui_update_only) {
        /* GDB_MESSAGE(GDB_GUI, " on_patches_selection: skipping further action\n"); */
        return;
    }

    GDB_MESSAGE(GDB_GUI, " on_patches_selection: patch %d selected\n", row);

    /* set all the patch edit widgets to match */
    update_voice_widgets_from_patch(&patches[row]);

    lo_send(osc_host_address, osc_program_path, "ii", 0, row);
}

void
on_voice_slider_change( GtkWidget *widget, gpointer data )
{
    int index = (int)data;
    struct xsynth_port_descriptor *xpd = &xsynth_port_description[index];
    float cval = GTK_ADJUSTMENT(widget)->value / 10.0f;
    float value;

    if (internal_gui_update_only) {
        /* GDB_MESSAGE(GDB_GUI, " on_voice_slider_change: skipping further action\n"); */
        return;
    }

    if (xpd->type == XSYNTH_PORT_TYPE_LINEAR) {

        value = (xpd->a * cval + xpd->b) * cval + xpd->c;  /* linear or quadratic */

    } else { /* XSYNTH_PORT_TYPE_LOGARITHMIC */

        value = xpd->a * exp(xpd->c * cval * log(xpd->b));

    }

    GDB_MESSAGE(GDB_GUI, " on_voice_slider_change: slider %d changed to %10.6f => %10.6f\n",
            index, GTK_ADJUSTMENT(widget)->value, value);

    lo_send(osc_host_address, osc_control_path, "if", index, value);
}

void
on_voice_detent_change( GtkWidget *widget, gpointer data )
{
    int index = (int)data;
    int value = lrintf(GTK_ADJUSTMENT(widget)->value);

    update_detent_label(index, value);

    if (internal_gui_update_only) {
        /* GDB_MESSAGE(GDB_GUI, " on_voice_detent_change: skipping further action\n"); */
        return;
    }

    GDB_MESSAGE(GDB_GUI, " on_voice_detent_change: detent %d changed to %d\n",
            index, value);

    lo_send(osc_host_address, osc_control_path, "if", index, (float)value);
}

void
on_voice_onoff_toggled( GtkWidget *widget, gpointer data )
{
    int index = (int)data;
    int state = GTK_TOGGLE_BUTTON (widget)->active;

    if (internal_gui_update_only) {
        /* GDB_MESSAGE(GDB_GUI, " on_voice_onoff_toggled: skipping further action\n"); */
        return;
    }

    GDB_MESSAGE(GDB_GUI, " on_voice_onoff_toggled: button %d changed to %s\n",
                index, (state ? "on" : "off"));

    lo_send(osc_host_address, osc_control_path, "if", index, (state ? 1.0f : 0.0f));
}

void
on_vcf_mode_activate(GtkWidget *widget, gpointer data)
{
    unsigned char mode = (unsigned char)(int)data;

    GDB_MESSAGE(GDB_GUI, " on_vcf_mode_activate: vcf mode '%d' selected\n", mode);

    lo_send(osc_host_address, osc_control_path, "if", XSYNTH_PORT_VCF_MODE, (float)mode);
#if !GTK_CHECK_VERSION(2, 0, 0)
    vcf_mode = mode;
#endif
}

void
on_test_note_slider_change(GtkWidget *widget, gpointer data)
{
    unsigned char value = lrintf(GTK_ADJUSTMENT(widget)->value);

    if ((int)data == 0) {  /* key */

        test_note_noteon_key = value;
        GDB_MESSAGE(GDB_GUI, " on_test_note_slider_change: new test note key %d\n", test_note_noteon_key);

    } else {  /* velocity */

        test_note_velocity = value;
        GDB_MESSAGE(GDB_GUI, " on_test_note_slider_change: new test note velocity %d\n", test_note_velocity);

    }
}

void
on_test_note_button_press(GtkWidget *widget, gpointer data)
{
    unsigned char midi[4];

    if ((int)data) {  /* button pressed */

        midi[0] = 0;
        midi[1] = 0x90;
        midi[2] = test_note_noteon_key;
        midi[3] = test_note_velocity;
        lo_send(osc_host_address, osc_midi_path, "m", midi);
        test_note_noteoff_key = test_note_noteon_key;

    } else { /* button released */

        midi[0] = 0;
        midi[1] = 0x80;
        midi[2] = test_note_noteoff_key;
        midi[3] = 0x40;
        lo_send(osc_host_address, osc_midi_path, "m", midi);

    }
}

void
on_edit_action_button_press(GtkWidget *widget, gpointer data)
{
    int i;

    GDB_MESSAGE(GDB_GUI, " on_edit_action_button_press: 'save changes' clicked\n");

    /* find the last non-init-voice patch, and set the save position to the
     * following patch */
    for (i = 128;
         i > 0 && gui_data_patch_compare(&patches[i - 1], &xsynth_init_voice);
         i--);
    if (i < 128)
        (GTK_ADJUSTMENT(edit_save_position_spin_adj))->value = (float)i;
    gtk_signal_emit_by_name (GTK_OBJECT (edit_save_position_spin_adj), "value_changed");

    gtk_widget_show(edit_save_position_window);
}

void
on_edit_save_position_ok( GtkWidget *widget, gpointer data )
{
    int position = lrintf(GTK_ADJUSTMENT(edit_save_position_spin_adj)->value);

    gtk_widget_hide(edit_save_position_window);

    GDB_MESSAGE(GDB_GUI, " on_edit_save_position_ok: position %d\n", position);

    /* set the patch to match all the edit widgets */
    update_patch_from_voice_widgets(&patches[position]);

    gui_data_mark_dirty_patch_sections(position, position);
    gui_data_send_dirty_patch_sections();

    rebuild_patches_clist();
}

void
on_edit_save_position_cancel( GtkWidget *widget, gpointer data )
{
    GDB_MESSAGE(GDB_GUI, " on_edit_save_position_cancel called\n");
    gtk_widget_hide(edit_save_position_window);
}

void
on_tuning_change(GtkWidget *widget, gpointer data)
{
    float value = GTK_ADJUSTMENT(widget)->value;

    if (internal_gui_update_only) {
        /* GDB_MESSAGE(GDB_GUI, " on_tuning_change: skipping further action\n"); */
        return;
    }

    GDB_MESSAGE(GDB_GUI, " on_tuning_change: tuning set to %10.6f\n", value);

    lo_send(osc_host_address, osc_control_path, "if", XSYNTH_PORT_TUNING, value);
}

void
on_polyphony_change(GtkWidget *widget, gpointer data)
{
    int polyphony = lrintf(GTK_ADJUSTMENT(widget)->value);
    char buffer[4];
    
    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_polyphony_change: skipping further action\n"); */
        return;
    }

    GDB_MESSAGE(GDB_GUI, " on_polyphony_change: polyphony set to %d\n", polyphony);

    snprintf(buffer, 4, "%d", polyphony);
    lo_send(osc_host_address, osc_configure_path, "ss", "polyphony", buffer);
}

void
on_mono_mode_activate(GtkWidget *widget, gpointer data)
{
    char *mode = data;

    GDB_MESSAGE(GDB_GUI, " on_mono_mode_activate: monophonic mode '%s' selected\n", mode);

    lo_send(osc_host_address, osc_configure_path, "ss", "monophonic", mode);
}

void
on_glide_mode_activate(GtkWidget *widget, gpointer data)
{
    char *mode = data;

    GDB_MESSAGE(GDB_GUI, " on_glide_mode_activate: glide mode '%s' selected\n", mode);

    lo_send(osc_host_address, osc_configure_path, "ss", "glide", mode);
}

void
on_bendrange_change(GtkWidget *widget, gpointer data)
{
    int bendrange = lrintf(GTK_ADJUSTMENT(widget)->value);
    char buffer[4];
    
    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_bendrange_change: skipping further action\n"); */
        return;
    }

    GDB_MESSAGE(GDB_GUI, " on_bendrange_change: bendrange set to %d\n", bendrange);

    snprintf(buffer, 4, "%d", bendrange);
    lo_send(osc_host_address, osc_configure_path, "ss", "bendrange", buffer);
}

void
display_notice(char *message1, char *message2)
{
    gtk_label_set_text (GTK_LABEL (notice_label_1), message1);
    gtk_label_set_text (GTK_LABEL (notice_label_2), message2);
    gtk_widget_show(notice_window);
}

void
on_notice_dismiss( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(notice_window);
}

void
update_detent_label(int index, int value)
{
    switch (index) {
      case XSYNTH_PORT_OSC1_WAVEFORM:
        set_waveform_pixmap(osc1_waveform_pixmap, value);
        break;

      case XSYNTH_PORT_OSC2_WAVEFORM:
        set_waveform_pixmap(osc2_waveform_pixmap, value);
        break;

      case XSYNTH_PORT_LFO_WAVEFORM:
        set_waveform_pixmap(lfo_waveform_pixmap, value);
        break;

      default:
        break;
    }
}

void
update_voice_widget(int port, float value)
{
    struct xsynth_port_descriptor *xpd;
    GtkAdjustment *adj;
    GtkWidget *button;
    float cval;
    int dval;

    if (port < XSYNTH_PORT_OSC1_PITCH || port >= XSYNTH_PORTS_COUNT) {
        return;
    }

    xpd = &xsynth_port_description[port];
    if (value < xpd->lower_bound)
        value = xpd->lower_bound;
    else if (value > xpd->upper_bound)
        value = xpd->upper_bound;
    
    internal_gui_update_only = 1;

    if (port == XSYNTH_PORT_TUNING) {  /* handle tuning specially, since it's not stored in patch */
        (GTK_ADJUSTMENT(tuning_adj))->value = value;
        gtk_signal_emit_by_name (tuning_adj, "value_changed");  /* causes call to on_tuning_change callback */

        internal_gui_update_only = 0;
        return;
    }

    switch (xpd->type) {

      case XSYNTH_PORT_TYPE_LINEAR:
        cval = (value - xpd->c) / xpd->b;  /* assume xpd->a == 0, which was always true for Xsynth */
        /* GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of %s to %f => %f\n", xpd->name, value, cval); */
        adj = (GtkAdjustment *)voice_widget[port];
        adj->value = cval * 10.0f;
        /* gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");        does not cause call to on_voice_slider_change callback */
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");  /* causes call to on_voice_slider_change callback */
        break;

      case XSYNTH_PORT_TYPE_LOGARITHMIC:
        cval = log(value / xpd->a) / (xpd->c * log(xpd->b));
        if (port == XSYNTH_PORT_OSC1_PITCH ||
            port == XSYNTH_PORT_OSC2_PITCH) {  /* oscillator pitch knobs go -10 to 10 */
            if (cval < -1.0 + 1.0e-6f)
                cval = -1.0f;
        } else {
            if (cval < 1.0e-6f)
                cval = 0.0f;
        }
        if (cval > 1.0f - 1.0e-6f)
            cval = 1.0f;
        /* GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of %s to %f => %f\n", xpd->name, value, cval); */
        adj = (GtkAdjustment *)voice_widget[port];
        adj->value = cval * 10.0f;
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");  /* causes call to on_voice_slider_change callback */
        break;

      case XSYNTH_PORT_TYPE_DETENT:
        dval = lrintf(value);
        /* GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of %s to %f => %d\n", xpd->name, value, dval); */
        adj = (GtkAdjustment *)voice_widget[port];
        adj->value = (float)dval;
        gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");  /* causes call to on_voice_detent_change callback */
        break;

      case XSYNTH_PORT_TYPE_ONOFF:
        dval = (value > 0.0001f ? 1 : 0);
        /* GDB_MESSAGE(GDB_GUI, " update_voice_widget: change of %s to %f => %d\n", xpd->name, value, dval); */
        button = (GtkWidget *)voice_widget[port];
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), dval); /* causes call to on_voice_onoff_toggled callback */
        break;

      case XSYNTH_PORT_TYPE_VCF_MODE:
        dval = lrintf(value);
        gtk_option_menu_set_history(GTK_OPTION_MENU (voice_widget[port]),
                                    dval);  /* updates optionmenu current selection,
                                             * without needing to send it a signal */
#if !GTK_CHECK_VERSION(2, 0, 0)
        vcf_mode = dval;
#endif
        break;

      default:
        break;
    }

    internal_gui_update_only = 0;
}

void
update_voice_widgets_from_patch(xsynth_patch_t *patch)
{
    update_voice_widget(XSYNTH_PORT_OSC1_PITCH,        patch->osc1_pitch);
    update_voice_widget(XSYNTH_PORT_OSC1_WAVEFORM,     (float)patch->osc1_waveform);
    update_voice_widget(XSYNTH_PORT_OSC1_PULSEWIDTH,   patch->osc1_pulsewidth);
    update_voice_widget(XSYNTH_PORT_OSC2_PITCH,        patch->osc2_pitch);
    update_voice_widget(XSYNTH_PORT_OSC2_WAVEFORM,     (float)patch->osc2_waveform);
    update_voice_widget(XSYNTH_PORT_OSC2_PULSEWIDTH,   patch->osc2_pulsewidth);
    update_voice_widget(XSYNTH_PORT_OSC_SYNC,          (float)patch->osc_sync);
    update_voice_widget(XSYNTH_PORT_OSC_BALANCE,       patch->osc_balance);
    update_voice_widget(XSYNTH_PORT_LFO_FREQUENCY,     patch->lfo_frequency);
    update_voice_widget(XSYNTH_PORT_LFO_WAVEFORM,      (float)patch->lfo_waveform);
    update_voice_widget(XSYNTH_PORT_LFO_AMOUNT_O,      patch->lfo_amount_o);
    update_voice_widget(XSYNTH_PORT_LFO_AMOUNT_F,      patch->lfo_amount_f);
    update_voice_widget(XSYNTH_PORT_EG1_ATTACK_TIME,   patch->eg1_attack_time);
    update_voice_widget(XSYNTH_PORT_EG1_DECAY_TIME,    patch->eg1_decay_time);
    update_voice_widget(XSYNTH_PORT_EG1_SUSTAIN_LEVEL, patch->eg1_sustain_level);
    update_voice_widget(XSYNTH_PORT_EG1_RELEASE_TIME,  patch->eg1_release_time);
    update_voice_widget(XSYNTH_PORT_EG1_VEL_SENS,      patch->eg1_vel_sens);
    update_voice_widget(XSYNTH_PORT_EG1_AMOUNT_O,      patch->eg1_amount_o);
    update_voice_widget(XSYNTH_PORT_EG1_AMOUNT_F,      patch->eg1_amount_f);
    update_voice_widget(XSYNTH_PORT_EG2_ATTACK_TIME,   patch->eg2_attack_time);
    update_voice_widget(XSYNTH_PORT_EG2_DECAY_TIME,    patch->eg2_decay_time);
    update_voice_widget(XSYNTH_PORT_EG2_SUSTAIN_LEVEL, patch->eg2_sustain_level);
    update_voice_widget(XSYNTH_PORT_EG2_RELEASE_TIME,  patch->eg2_release_time);
    update_voice_widget(XSYNTH_PORT_EG2_VEL_SENS,      patch->eg2_vel_sens);
    update_voice_widget(XSYNTH_PORT_EG2_AMOUNT_O,      patch->eg2_amount_o);
    update_voice_widget(XSYNTH_PORT_EG2_AMOUNT_F,      patch->eg2_amount_f);
    update_voice_widget(XSYNTH_PORT_VCF_CUTOFF,        patch->vcf_cutoff);
    update_voice_widget(XSYNTH_PORT_VCF_QRES,          patch->vcf_qres);
    update_voice_widget(XSYNTH_PORT_VCF_MODE,          (float)patch->vcf_mode);
    update_voice_widget(XSYNTH_PORT_GLIDE_TIME,        patch->glide_time);
    update_voice_widget(XSYNTH_PORT_VOLUME,            patch->volume);
    gtk_entry_set_text(GTK_ENTRY(name_entry), patch->name);
}

void
update_from_program_select(int bank, int program)
{
    internal_gui_update_only = 1;
    gtk_clist_select_row (GTK_CLIST(patches_clist), program, 0);
    internal_gui_update_only = 0;

    update_voice_widgets_from_patch(&patches[program]);
}

static float
get_value_from_slider(int index)
{
    struct xsynth_port_descriptor *xpd = &xsynth_port_description[index];
    float cval = GTK_ADJUSTMENT(voice_widget[index])->value / 10.0f;

    if (xpd->type == XSYNTH_PORT_TYPE_LINEAR) {

        return (xpd->a * cval + xpd->b) * cval + xpd->c;  /* linear or quadratic */

    } else { /* XSYNTH_PORT_TYPE_LOGARITHMIC */

        return xpd->a * exp(xpd->c * cval * log(xpd->b));

    }
}

static unsigned char
get_value_from_detent(int index)
{
    return lrintf(GTK_ADJUSTMENT(voice_widget[index])->value);
}

static unsigned char
get_value_from_onoff(int index)
{
    return (GTK_TOGGLE_BUTTON (voice_widget[index])->active ? 1 : 0);
}

static unsigned char
get_value_from_opmenu(int index)
{
#if GTK_CHECK_VERSION(2, 0, 0)
    return gtk_option_menu_get_history(GTK_OPTION_MENU (voice_widget[index]));
#else
    return vcf_mode;
#endif
}

void
update_patch_from_voice_widgets(xsynth_patch_t *patch)
{
    int i;

    patch->osc1_pitch           = get_value_from_slider(XSYNTH_PORT_OSC1_PITCH);        
    patch->osc1_waveform        = get_value_from_detent(XSYNTH_PORT_OSC1_WAVEFORM);     
    patch->osc1_pulsewidth      = get_value_from_slider(XSYNTH_PORT_OSC1_PULSEWIDTH);   
    patch->osc2_pitch           = get_value_from_slider(XSYNTH_PORT_OSC2_PITCH);        
    patch->osc2_waveform        = get_value_from_detent(XSYNTH_PORT_OSC2_WAVEFORM);     
    patch->osc2_pulsewidth      = get_value_from_slider(XSYNTH_PORT_OSC2_PULSEWIDTH);   
    patch->osc_sync             = get_value_from_onoff(XSYNTH_PORT_OSC_SYNC);          
    patch->osc_balance          = get_value_from_slider(XSYNTH_PORT_OSC_BALANCE);       
    patch->lfo_frequency        = get_value_from_slider(XSYNTH_PORT_LFO_FREQUENCY);     
    patch->lfo_waveform         = get_value_from_detent(XSYNTH_PORT_LFO_WAVEFORM);      
    patch->lfo_amount_o         = get_value_from_slider(XSYNTH_PORT_LFO_AMOUNT_O);      
    patch->lfo_amount_f         = get_value_from_slider(XSYNTH_PORT_LFO_AMOUNT_F);      
    patch->eg1_attack_time      = get_value_from_slider(XSYNTH_PORT_EG1_ATTACK_TIME);   
    patch->eg1_decay_time       = get_value_from_slider(XSYNTH_PORT_EG1_DECAY_TIME);    
    patch->eg1_sustain_level    = get_value_from_slider(XSYNTH_PORT_EG1_SUSTAIN_LEVEL); 
    patch->eg1_release_time     = get_value_from_slider(XSYNTH_PORT_EG1_RELEASE_TIME);  
    patch->eg1_vel_sens         = get_value_from_slider(XSYNTH_PORT_EG1_VEL_SENS);
    patch->eg1_amount_o         = get_value_from_slider(XSYNTH_PORT_EG1_AMOUNT_O);      
    patch->eg1_amount_f         = get_value_from_slider(XSYNTH_PORT_EG1_AMOUNT_F);      
    patch->eg2_attack_time      = get_value_from_slider(XSYNTH_PORT_EG2_ATTACK_TIME);   
    patch->eg2_decay_time       = get_value_from_slider(XSYNTH_PORT_EG2_DECAY_TIME);    
    patch->eg2_sustain_level    = get_value_from_slider(XSYNTH_PORT_EG2_SUSTAIN_LEVEL); 
    patch->eg2_release_time     = get_value_from_slider(XSYNTH_PORT_EG2_RELEASE_TIME);  
    patch->eg2_vel_sens         = get_value_from_slider(XSYNTH_PORT_EG2_VEL_SENS);
    patch->eg2_amount_o         = get_value_from_slider(XSYNTH_PORT_EG2_AMOUNT_O);      
    patch->eg2_amount_f         = get_value_from_slider(XSYNTH_PORT_EG2_AMOUNT_F);      
    patch->vcf_cutoff           = get_value_from_slider(XSYNTH_PORT_VCF_CUTOFF);        
    patch->vcf_qres             = get_value_from_slider(XSYNTH_PORT_VCF_QRES);          
    patch->vcf_mode             = get_value_from_opmenu(XSYNTH_PORT_VCF_MODE);
    patch->glide_time           = get_value_from_slider(XSYNTH_PORT_GLIDE_TIME);        
    patch->volume               = get_value_from_slider(XSYNTH_PORT_VOLUME);

    strncpy(patch->name, gtk_entry_get_text(GTK_ENTRY(name_entry)), 30);
    patch->name[30] = 0;
    /* trim trailing spaces */
    i = strlen(patch->name);
    while(i && patch->name[i - 1] == ' ') i--;
    patch->name[i] = 0;
}

void
update_patches(const char *key, const char *value)
{
    int section = key[7] - '0';

    GDB_MESSAGE(GDB_OSC, ": update_patches: received new '%s'\n", key);

    if (section < 0 || section > 3)
        return;

    if (!xsynth_data_decode_patches(value, &patches[section * 32])) {
        GDB_MESSAGE(GDB_OSC, " update_patches: corrupt data!\n");
        return;
    }

    patch_section_dirty[section] = 0;

    rebuild_patches_clist();
    /* internal_gui_update_only = 1;
     * gtk_clist_select_row (GTK_CLIST(patches_clist), current_program, 0);
     * internal_gui_update_only = 0; */
}

void
update_polyphony(const char *value)
{
    int poly = atoi(value);

    GDB_MESSAGE(GDB_OSC, ": update_polyphony called with '%s'\n", value);

    if (poly > 0 && poly < XSYNTH_MAX_POLYPHONY) {

        internal_gui_update_only = 1;

        GTK_ADJUSTMENT(polyphony_adj)->value = (float)poly;
        gtk_signal_emit_by_name (GTK_OBJECT (polyphony_adj), "value_changed");  /* causes call to on_polyphony_change callback */

        internal_gui_update_only = 0;
    }
}

void
update_monophonic(const char *value)
{
    int index;

    GDB_MESSAGE(GDB_OSC, ": update_monophonic called with '%s'\n", value);

    if (!strcmp(value, "off")) {
        index = 0;
    } else if (!strcmp(value, "on")) {
        index = 1;
    } else if (!strcmp(value, "once")) {
        index = 2;
    } else if (!strcmp(value, "both")) {
        index = 3;
    } else {
        return;
    }

    gtk_option_menu_set_history(GTK_OPTION_MENU (monophonic_option_menu),
                                index);  /* updates optionmenu current selection,
                                          * without needing to send it a signal */
}

void
update_glide(const char *value)
{
    int index;

    GDB_MESSAGE(GDB_OSC, ": update_glide called with '%s'\n", value);

    if (!strcmp(value, "legato")) {
        index = 0;
    } else if (!strcmp(value, "initial")) {
        index = 1;
    } else if (!strcmp(value, "always")) {
        index = 2;
    } else if (!strcmp(value, "leftover")) {
        index = 3;
    } else if (!strcmp(value, "off")) {
        index = 4;
    } else {
        return;
    }

    gtk_option_menu_set_history(GTK_OPTION_MENU (glide_option_menu),
                                index);  /* updates optionmenu current selection,
                                          * without needing to send it a signal */
}

void
update_bendrange(const char *value)
{
    int range = atoi(value);

    GDB_MESSAGE(GDB_OSC, ": update_bendrange called with '%s'\n", value);

    if (range >= 0 && range <= 12) {

        internal_gui_update_only = 1;

        GTK_ADJUSTMENT(bendrange_adj)->value = (float)range;
        gtk_signal_emit_by_name (GTK_OBJECT (bendrange_adj), "value_changed");  /* causes call to on_bendrange_change callback */

        internal_gui_update_only = 0;
    }
}

void
rebuild_patches_clist(void)
{
    char number[4], name[31];
    char *data[2] = { number, name };
    int i;

    GDB_MESSAGE(GDB_GUI, ": rebuild_patches_clist called\n");

    gtk_clist_freeze(GTK_CLIST(patches_clist));
    gtk_clist_clear(GTK_CLIST(patches_clist));
    for (i = 0; i < 128; i++) {
        snprintf(number, 4, "%d", i);
        strncpy(name, patches[i].name, 31);
        gtk_clist_append(GTK_CLIST(patches_clist), data);
    }
#if GTK_CHECK_VERSION(2, 0, 0)
    /* kick GTK+ 2.4.x in the pants.... */
    gtk_signal_emit_by_name (GTK_OBJECT (patches_clist), "check-resize");
#endif    
    gtk_clist_thaw(GTK_CLIST(patches_clist));
}

