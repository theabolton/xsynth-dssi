/* Xsynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004 Sean Bolton and others.
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
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
                      unsigned char *old_patch, int unpack_name,
                      unsigned long bank, unsigned long program)
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
    xsynth_patch->eg1_amount_o      = le_float(old_patch + 108);
    xsynth_patch->eg1_amount_f      = le_float(old_patch + 116);

    /* EG 2 */
    xsynth_patch->eg2_attack_time   = le_float(old_patch + 124);
    xsynth_patch->eg2_decay_time    = le_float(old_patch + 132);
    xsynth_patch->eg2_sustain_level = le_float(old_patch + 140);
    xsynth_patch->eg2_release_time  = le_float(old_patch + 148);
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
    xsynth_patch->vcf_4pole         = old_patch[208];

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
    int i;

    fprintf(file, "# Xsynth-dssi patch\n");
    fprintf(file, "xsynth-dssi patch format 0 begin\n");

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

    fprintf(file, "osc1 %.6f %d %.6f\n", patch->osc1_pitch,
            patch->osc1_waveform, patch->osc1_pulsewidth);
    fprintf(file, "osc2 %.6f %d %.6f\n", patch->osc2_pitch,
            patch->osc2_waveform, patch->osc2_pulsewidth);
    fprintf(file, "sync %d\n", patch->osc_sync);
    fprintf(file, "balance %.6f\n", patch->osc_balance);

    fprintf(file, "lfo %.6f %d %.6f %.6f\n", patch->lfo_frequency,
            patch->lfo_waveform, patch->lfo_amount_o, patch->lfo_amount_f);

    fprintf(file, "eg1 %.6f %.6f %.6f %.6f %.6f %.6f\n",
            patch->eg1_attack_time, patch->eg1_decay_time,
            patch->eg1_sustain_level, patch->eg1_release_time,
            patch->eg1_amount_o, patch->eg1_amount_f);
    fprintf(file, "eg2 %.6f %.6f %.6f %.6f %.6f %.6f\n",
            patch->eg2_attack_time, patch->eg2_decay_time,
            patch->eg2_sustain_level, patch->eg2_release_time,
            patch->eg2_amount_o, patch->eg2_amount_f);

    fprintf(file, "vcf %.6f %.6f %d\n", patch->vcf_cutoff, patch->vcf_qres,
            patch->vcf_4pole);

    fprintf(file, "glide %.6f\n", patch->glide_time);
    fprintf(file, "volume %.6f\n", patch->volume);

    fprintf(file, "xsynth-dssi patch end\n");

    return 1;  /* -FIX- error handling yet to be implemented */
}

int
gui_data_save_dirty_patches_to_tmp(void)
{
    FILE *fh;
    int i;

    if ((fh = fopen(patches_tmp_filename, "wb")) == NULL) {
        return 0;
    }
    for (i = 0; i < patch_count; i++) {
        if (!gui_data_write_patch(fh, &patches[i])) {
            fclose(fh);
            return 0;
        }
    }
    fclose(fh);
    return 1;
}

int
gui_data_save(char *filename, char **message)
{
    FILE *fh;
    int i;
    char buffer[20];

    GDB_MESSAGE(GDB_IO, " gui_data_save: attempting to save '%s'\n", filename);

    if ((fh = fopen(filename, "wb")) == NULL) {
        if (message) *message = strdup("could not open file for writing");
        return 0;
    }
    for (i = 0; i < patch_count; i++) {
        if (!gui_data_write_patch(fh, &patches[i])) {
            fclose(fh);
            if (message) *message = strdup("error while writing file");
            return 0;
        }
    }
    fclose(fh);

    if (message) {
        snprintf(buffer, 20, "wrote %d patches", patch_count);
        *message = strdup(buffer);
    }
    return 1;
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

    /* -FIX- implement bank support */
    if (!patches) {
        if (!(patches = (xsynth_patch_t *)malloc(128 * sizeof(xsynth_patch_t))))
            if (message) *message = strdup("out of memory");
            return 0;
    }

    if ((fh = fopen(filename, "rb")) == NULL) {
        if (message) *message = strdup("could not open file for reading");
        return 0;
    }
    while (index < 128 &&
           xsynth_data_read_patch(fh, &patches[index], 0, index)) {
        count++;
        index++;
    }
    fclose(fh);

    if (!count) {
        if (message) *message = strdup("no patches recognized");
        return 0;
    }

    if (position == 0 && count >= patch_count)
        patches_dirty = 0;
    else
        patches_dirty = 1;
    if (index > patch_count)
        patch_count = index;

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
    if (!patches) {
        if (!(patches = (xsynth_patch_t *)malloc(128 * sizeof(xsynth_patch_t))))
            return;
    }

    memcpy(patches, friendly_patches, friendly_patch_count * sizeof(xsynth_patch_t));

    patch_count = friendly_patch_count;
}

