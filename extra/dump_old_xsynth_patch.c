/* Xsynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
 *
 * Portions of this file came from Steve Brookes' Xsynth,
 * copyright (C) 1999 S. J. Brookes.
 * Portions of this file may have come from Chris Cannam and Steve
 * Harris's public domain DSSI example code.
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

/*
 * This program will read a patch file saved by Xsynth 1.0.2 and output
 * it in a form suitable for loading into Xsynth-DSSI.  Example usage:
 *
 * $ dump_old_xsynth_patch mypatch.Xpatch >mypatch.Xsynth
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "xsynth.h"
#include "xsynth_synth.h"
#include "xsynth_voice.h"

#define XSYNTH_VOICE_SIZE_WITH_NAME  224

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
 * xsynth_voice_import_patch
 *
 * import a patch in old Xsynth format
 */
void
xsynth_voice_import_patch(xsynth_patch_t *xsynth_patch,
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
xsynth_voice_write_patch(FILE *file, xsynth_patch_t *patch)
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


int
main(int argc, char *argv[])
{
    FILE *fh;
    unsigned char buffer[8192];
    size_t length;
    int i;
    xsynth_patch_t patch;

    fh = fopen(argv[1], "rb");
    length = fread(buffer, 1, 8192, fh);
    for (i = 0; i < length; i += XSYNTH_VOICE_SIZE_WITH_NAME) {
        xsynth_voice_import_patch(&patch, buffer + i, (length - i > 209), 0, i / XSYNTH_VOICE_SIZE_WITH_NAME);
        xsynth_voice_write_patch(stdout, &patch);
    }

    return 0;
}

