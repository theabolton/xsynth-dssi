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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "xsynth_types.h"
#include "xsynth.h"
#include "xsynth_synth.h"
#include "xsynth_voice.h"
#include "gui_main.h"
#include "gui_data.h"

#ifdef WORDS_BIGENDIAN
static float
le_float(unsigned char *p)
{
    unsigned char c[4];
    c[3] = *p++;
    c[2] = *p++;
    c[1] = *p++;
    c[0] = *p;
    return *((float *)c);
}
#else  /* little-endian: */
static float
le_float(unsigned char *p)
{
    return *((float *)p);
}
#endif

static unsigned char
waveform_bit_to_number(unsigned char bit)
{
    switch (bit) {
      case 0x01:
      default:
        return 0;
      case 0x02:
        return 1;
      case 0x04:
        return 2;
      case 0x08:
        return 3;
      case 0x10:
        return 4;
      case 0x20:
        return 5;
    }
}

/*
 * gui_data_import_patch
 *
 * import a patch in old Xsynth format
 */
void
gui_data_import_patch(xsynth_patch_t *xsynth_patch,
                      unsigned char *old_patch, int unpack_name)
{
    if (!unpack_name) {
        strcpy(xsynth_patch->name, "imported patch");
    }
    
    /* Unpack patch data */
    /* Continuous knobs */
    /* VCO 1 */
    xsynth_patch->osc1_pitch        = le_float(old_patch + 4);
    xsynth_patch->osc1_pulsewidth   = le_float(old_patch + 12);

    /* VCO 2 */
    xsynth_patch->osc2_pitch        = le_float(old_patch + 20);
    xsynth_patch->osc2_pulsewidth   = le_float(old_patch + 28);

    /* LFO */
    xsynth_patch->lfo_frequency     = le_float(old_patch + 36);
    xsynth_patch->lfo_amount_o      = le_float(old_patch + 44);
    xsynth_patch->lfo_amount_f      = le_float(old_patch + 52);

    /* BALANCE */
    xsynth_patch->osc_balance       = le_float(old_patch + 60);

    /* PORTAMENTO */
    /* glide_time: compensate for different control-value-recalulation rates */
    xsynth_patch->glide_time        = 1.0f - pow((1.0f - le_float(old_patch + 68)),
                                                 (float)XSYNTH_NUGGET_SIZE / 256.0f);

    /* EG 1 */
    xsynth_patch->eg1_attack_time   = le_float(old_patch + 76);
    xsynth_patch->eg1_decay_time    = le_float(old_patch + 84);
    xsynth_patch->eg1_sustain_level = le_float(old_patch + 92);
    xsynth_patch->eg1_release_time  = le_float(old_patch + 100);
    xsynth_patch->eg1_vel_sens      = 0.0f;
    xsynth_patch->eg1_amount_o      = le_float(old_patch + 108);
    xsynth_patch->eg1_amount_f      = le_float(old_patch + 116);

    /* EG 2 */
    xsynth_patch->eg2_attack_time   = le_float(old_patch + 124);
    xsynth_patch->eg2_decay_time    = le_float(old_patch + 132);
    xsynth_patch->eg2_sustain_level = le_float(old_patch + 140);
    xsynth_patch->eg2_release_time  = le_float(old_patch + 148);
    xsynth_patch->eg2_vel_sens      = 0.0f;
    xsynth_patch->eg2_amount_o      = le_float(old_patch + 156);
    xsynth_patch->eg2_amount_f      = le_float(old_patch + 164);

    /* VCF */
    xsynth_patch->vcf_cutoff        = le_float(old_patch + 172);
    xsynth_patch->vcf_qres          = 2.0f - le_float(old_patch + 180);

    /* VOLUME */
    xsynth_patch->volume            = le_float(old_patch + 188);

    /* Detent knobs */
    xsynth_patch->osc1_waveform     = waveform_bit_to_number(old_patch[196]);
    xsynth_patch->osc2_waveform     = waveform_bit_to_number(old_patch[201]);
    xsynth_patch->lfo_waveform      = waveform_bit_to_number(old_patch[206]);

    /* Buttons */
    xsynth_patch->osc_sync          = old_patch[207];
    xsynth_patch->vcf_mode          = old_patch[208];

    /* Name */
    if (unpack_name) {
        int i;

        for (i = 0; i < 15; i++) {  /* Yes, 15: old banks had only 15 character names */
            xsynth_patch->name[i] = *(old_patch + 209 + i);
        }
        xsynth_patch->name[15] = '\0';
    }
}

int
gui_data_write_patch(FILE *file, xsynth_patch_t *patch)
{
    int format, i;

    if (patch->eg1_vel_sens < 1e-6f &&
        patch->eg2_vel_sens < 1e-6f)
        format = 0;
    else
        format = 1;

    fprintf(file, "# Xsynth-dssi patch\n");
    fprintf(file, "xsynth-dssi patch format %d begin\n", format);

    fprintf(file, "name ");
    for (i = 0; i < 30; i++) {
        if (!patch->name[i]) {
            break;
        } else if (patch->name[i] < 33 || patch->name[i] > 126 ||
                   patch->name[i] == '%') {
            fprintf(file, "%%%02x", patch->name[i]);
        } else {
            fputc(patch->name[i], file);
        }
    }
    fprintf(file, "\n");

    fprintf(file, "osc1 %.6g %d %.6g\n", patch->osc1_pitch,
            patch->osc1_waveform, patch->osc1_pulsewidth);
    fprintf(file, "osc2 %.6g %d %.6g\n", patch->osc2_pitch,
            patch->osc2_waveform, patch->osc2_pulsewidth);
    fprintf(file, "sync %d\n", patch->osc_sync);
    fprintf(file, "balance %.6g\n", patch->osc_balance);

    fprintf(file, "lfo %.6g %d %.6g %.6g\n", patch->lfo_frequency,
            patch->lfo_waveform, patch->lfo_amount_o, patch->lfo_amount_f);

    if (format == 0) {  /* backward compatible */

        fprintf(file, "eg1 %.6g %.6g %.6g %.6g %.6g %.6g\n",
                patch->eg1_attack_time, patch->eg1_decay_time,
                patch->eg1_sustain_level, patch->eg1_release_time,
                patch->eg1_amount_o, patch->eg1_amount_f);
        fprintf(file, "eg2 %.6g %.6g %.6g %.6g %.6g %.6g\n",
                patch->eg2_attack_time, patch->eg2_decay_time,
                patch->eg2_sustain_level, patch->eg2_release_time,
                patch->eg2_amount_o, patch->eg2_amount_f);

    } else {

        fprintf(file, "eg1 %.6g %.6g %.6g %.6g %.6g %.6g %.6g\n",
                patch->eg1_attack_time, patch->eg1_decay_time,
                patch->eg1_sustain_level, patch->eg1_release_time,
                patch->eg1_vel_sens, patch->eg1_amount_o, patch->eg1_amount_f);
        fprintf(file, "eg2 %.6g %.6g %.6g %.6g %.6g %.6g %.6g\n",
                patch->eg2_attack_time, patch->eg2_decay_time,
                patch->eg2_sustain_level, patch->eg2_release_time,
                patch->eg2_vel_sens, patch->eg2_amount_o, patch->eg2_amount_f);
    }

    fprintf(file, "vcf %.6g %.6g %d\n", patch->vcf_cutoff, patch->vcf_qres,
            patch->vcf_mode);

    fprintf(file, "glide %.6g\n", patch->glide_time);
    fprintf(file, "volume %.6g\n", patch->volume);

    fprintf(file, "xsynth-dssi patch end\n");

    return 1;  /* -FIX- error handling yet to be implemented */
}

/*
 * gui_data_save
 */
int
gui_data_save(char *filename, int start, int end, char **message)
{
    FILE *fh;
    int i;
    char buffer[20];

    GDB_MESSAGE(GDB_IO, " gui_data_save: attempting to save '%s'\n", filename);

    if ((fh = fopen(filename, "wb")) == NULL) {
        if (message) *message = strdup("could not open file for writing");
        return 0;
    }
    for (i = start; i <= end; i++) {
        if (!gui_data_write_patch(fh, &patches[i])) {
            fclose(fh);
            if (message) *message = strdup("error while writing file");
            return 0;
        }
    }
    fclose(fh);

    if (message) {
        snprintf(buffer, 20, "wrote %d patches", end - start + 1);
        *message = strdup(buffer);
    }
    return 1;
}

/*
 * gui_data_mark_dirty_patch_sections
 */
void
gui_data_mark_dirty_patch_sections(int start_patch, int end_patch)
{
    int i, block;
    for (i = start_patch; i <= end_patch; ) {
        block = i >> 5;
        patch_section_dirty[block] = 1;
        i = (block + 1) << 5;
    }
}

/*
 * gui_data_load
 */
int
gui_data_load(const char *filename, int position, char **message)
{
    FILE *fh;
    int count = 0;
    int index = position;
    char buffer[20];

    GDB_MESSAGE(GDB_IO, " gui_data_load: attempting to load '%s'\n", filename);

    if ((fh = fopen(filename, "rb")) == NULL) {
        if (message) *message = strdup("could not open file for reading");
        return 0;
    }
    while (index < 128 &&
           xsynth_data_read_patch(fh, &patches[index])) {
        count++;
        index++;
    }
    fclose(fh);

    if (!count) {
        if (message) *message = strdup("no patches recognized");
        return 0;
    }

    gui_data_mark_dirty_patch_sections(position, position + count - 1);

    if (message) {
        snprintf(buffer, 20, "loaded %d patches", count);
        *message = strdup(buffer);
    }
    return count;
}

/*
 * gui_data_friendly_patches
 *
 * give the new user a default set of good patches to get started with
 */
void
gui_data_friendly_patches(void)
{
    int i;

    if (!(patches = (xsynth_patch_t *)malloc(128 * sizeof(xsynth_patch_t)))) {
        GDB_MESSAGE(-1, " gui_data_friendly_patches fatal: out of memory!\n");
        exit(1);
    }

    memcpy(patches, friendly_patches, friendly_patch_count * sizeof(xsynth_patch_t));

    for (i = friendly_patch_count; i < 128; i++) {
        memcpy(&patches[i], &xsynth_init_voice, sizeof(xsynth_patch_t));
    }

    patch_section_dirty[0] = 0;
    patch_section_dirty[1] = 0;
    patch_section_dirty[2] = 0;
    patch_section_dirty[3] = 0;
}

static void
encode_patch(xsynth_patch_t *patch, char **ep, int maxlen)
{
    int i, n;

    for (i = 0; maxlen > 3 && i < 30; i++) {
        if (!patch->name[i]) {
            break;
        } else if (patch->name[i] < 33 || patch->name[i] > 126 ||
                   patch->name[i] == '%') {
            sprintf(*ep, "%%%02x", patch->name[i]);
            *ep += 3;
            maxlen -= 3;
        } else {
            **ep = patch->name[i];
            (*ep)++;
            maxlen--;
        }
    }

    snprintf(*ep, maxlen,
             " %.6g %d %.6g %.6g %d %.6g %d %.6g"   /* through osc balance */
             " %.6g %d %.6g %.6g"                   /* through lfo */
             " %.6g %.6g %.6g %.6g %.6g %.6g %.6g"  /* through eg1 */
             " %.6g %.6g %.6g %.6g %.6g %.6g %.6g"  /* through eg2 */
             " %.6g %.6g %d %.6g %.6g %n",
             patch->osc1_pitch, patch->osc1_waveform, patch->osc1_pulsewidth,
             patch->osc2_pitch, patch->osc2_waveform, patch->osc2_pulsewidth,
             patch->osc_sync, patch->osc_balance,
             patch->lfo_frequency, patch->lfo_waveform, patch->lfo_amount_o,
             patch->lfo_amount_f,
             patch->eg1_attack_time, patch->eg1_decay_time,
             patch->eg1_sustain_level, patch->eg1_release_time,
             patch->eg1_vel_sens, patch->eg1_amount_o, patch->eg1_amount_f,
             patch->eg2_attack_time, patch->eg2_decay_time,
             patch->eg2_sustain_level, patch->eg2_release_time,
             patch->eg2_vel_sens, patch->eg2_amount_o, patch->eg2_amount_f,
             patch->vcf_cutoff, patch->vcf_qres, patch->vcf_mode,
             patch->glide_time, patch->volume, &n);

    *ep += n;
}

static int
send_patch_section(int section, xsynth_patch_t *block)
{
    int i;
    char *e = (char *)malloc(16000);
    char *ep = e,
         *ee = e + 16000;
    char key[9];

    if (!e) return 0;

    sprintf(ep, "Xp0 ");
    ep += 4;

    for (i = 0; i < 32; i++)
        encode_patch(&block[i], &ep, ee - ep);

    if (ee - ep < 4) {  /* no room left (shouldn't happen) */
        free(e);
        return 0;
    }
    sprintf(ep, "end");

    snprintf(key, 9, "patches%d", section);
    lo_send(osc_host_address, osc_configure_path, "ss", key, e);

    free(e);

    return 1;
}

/*
 * gui_data_send_dirty_patch_sections
 */
void
gui_data_send_dirty_patch_sections(void)
{
    int section;
    for (section = 0; section < 4; section++) {
        if (patch_section_dirty[section] &&
            send_patch_section(section, &patches[section << 5])) {
            patch_section_dirty[section] = 0;
        }
    }
}

/*
 * gui_data_patch_compare
 *
 * returns true if two patches are the same
 */
int
gui_data_patch_compare(xsynth_patch_t *patch1, xsynth_patch_t *patch2)
{
    if (strcmp(patch1->name, patch2->name))
        return 0;

    if (patch1->osc1_pitch        != patch2->osc1_pitch        ||
        patch1->osc1_waveform     != patch2->osc1_waveform     ||
        patch1->osc1_pulsewidth   != patch2->osc1_pulsewidth   ||
        patch1->osc2_pitch        != patch2->osc2_pitch        ||
        patch1->osc2_waveform     != patch2->osc2_waveform     ||
        patch1->osc2_pulsewidth   != patch2->osc2_pulsewidth   ||
        patch1->osc_sync          != patch2->osc_sync          ||
        patch1->osc_balance       != patch2->osc_balance       ||
        patch1->lfo_frequency     != patch2->lfo_frequency     ||
        patch1->lfo_waveform      != patch2->lfo_waveform      ||
        patch1->lfo_amount_o      != patch2->lfo_amount_o      ||
        patch1->lfo_amount_f      != patch2->lfo_amount_f      ||
        patch1->eg1_attack_time   != patch2->eg1_attack_time   ||
        patch1->eg1_decay_time    != patch2->eg1_decay_time    ||
        patch1->eg1_sustain_level != patch2->eg1_sustain_level ||
        patch1->eg1_release_time  != patch2->eg1_release_time  ||
        patch1->eg1_vel_sens      != patch2->eg1_vel_sens      ||
        patch1->eg1_amount_o      != patch2->eg1_amount_o      ||
        patch1->eg1_amount_f      != patch2->eg1_amount_f      ||
        patch1->eg2_attack_time   != patch2->eg2_attack_time   ||
        patch1->eg2_decay_time    != patch2->eg2_decay_time    ||
        patch1->eg2_sustain_level != patch2->eg2_sustain_level ||
        patch1->eg2_release_time  != patch2->eg2_release_time  ||
        patch1->eg2_vel_sens      != patch2->eg2_vel_sens      ||
        patch1->eg2_amount_o      != patch2->eg2_amount_o      ||
        patch1->eg2_amount_f      != patch2->eg2_amount_f      ||
        patch1->vcf_cutoff        != patch2->vcf_cutoff        ||
        patch1->vcf_qres          != patch2->vcf_qres          ||
        patch1->vcf_mode          != patch2->vcf_mode          ||
        patch1->glide_time        != patch2->glide_time        ||
        patch1->volume            != patch2->volume)
        return 0;

    return 1;
}

