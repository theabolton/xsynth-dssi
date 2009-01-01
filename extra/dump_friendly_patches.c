/* Xsynth DSSI software synthesizer plugin
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

/* This program reads an Xsynth-DSSI patch file, and outputs it
 * in C structure format suitable for include in
 * gui_friendly_patches.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "xsynth.h"
#include "xsynth_synth.h"
#include "xsynth_voice.h"

// xsynth_patch_t xsynth_init_voice = {
//     "  <-->",
//     1.0f, 0, 0.5f,                            /* osc1 */
//     1.0f, 0, 0.5f,                            /* osc2 */
//     0,                                        /* sync */
//     0.5f,                                     /* balance */
//     0.1f, 0, 0.0f, 0.0f,                      /* lfo */
//     0.1f, 0.1f, 1.0f, 0.1f, 0.0f, 0.0f, 0.0f, /* eg1 */
//     0.1f, 0.1f, 1.0f, 0.1f, 0.0f, 0.0f, 0.0f, /* eg2 */
//     50.0f, 0.0f, 0,                           /* vcf */
//     0.984375f,                                /* glide */
//     0.5f                                      /* volume */
// };

int
xsynth_voice_write_patch_as_c(FILE *file, xsynth_patch_t *patch)
{
    int i;

    fprintf(file, "    {\n");

    fprintf(file, "        \"");
    for (i = 0; i < 30; i++) {
        if (!patch->name[i]) {
            break;
        } else if (patch->name[i] < 32 || patch->name[i] > 126 ||
                   patch->name[i] == '"' || patch->name[i] == '\\') {
            fprintf(file, "\\%03o", patch->name[i]);
        } else {
            fputc(patch->name[i], file);
        }
    }
    fprintf(file, "\",\n");

    fprintf(file, "        %.6g, %d, %.6g,\n", patch->osc1_pitch,
            patch->osc1_waveform, patch->osc1_pulsewidth);
    fprintf(file, "        %.6g, %d, %.6g,\n", patch->osc2_pitch,
            patch->osc2_waveform, patch->osc2_pulsewidth);
    fprintf(file, "        %d, %.6g,\n", patch->osc_sync, patch->osc_balance);

    fprintf(file, "        %.6g, %d, %.6g, %.6g,\n", patch->lfo_frequency,
            patch->lfo_waveform, patch->lfo_amount_o, patch->lfo_amount_f);

    fprintf(file, "        %.6g, %.6g, %.6g, %.6g, %.6g, %.6g, %.6g,\n",
            patch->eg1_attack_time, patch->eg1_decay_time,
            patch->eg1_sustain_level, patch->eg1_release_time,
            patch->eg1_vel_sens, patch->eg1_amount_o, patch->eg1_amount_f);
    fprintf(file, "        %.6g, %.6g, %.6g, %.6g, %.6g, %.6g, %.6g,\n",
            patch->eg2_attack_time, patch->eg2_decay_time,
            patch->eg2_sustain_level, patch->eg2_release_time,
            patch->eg2_vel_sens, patch->eg2_amount_o, patch->eg2_amount_f);

    fprintf(file, "        %.6g, %.6g, %d,\n", patch->vcf_cutoff,
            patch->vcf_qres, patch->vcf_mode);

    fprintf(file, "        %.6g, %.6g\n", patch->glide_time, patch->volume);

    fprintf(file, "    },\n");

    return 1;  /* -FIX- error handling yet to be implemented */
}

static int
is_comment(char *buf)  /* line is blank, whitespace, or first non-whitespace character is '#' */
{
    int i = 0;

    while (buf[i]) {
        if (buf[i] == '#') return 1;
        if (buf[i] == '\n') return 1;
        if (buf[i] != ' ' && buf[i] != '\t') return 0;
        i++;
    }
    return 1;
}

static void
parse_name(char *buf, char *name)
{
    int i = 0, o = 0;
    unsigned int t;

    while (buf[i] && o < 30) {
        if (buf[i] < 32 || buf[i] > 126) {
            break;
        } else if (buf[i] == '%') {
            if (buf[i + 1] && buf[i + 2] && sscanf(buf + i + 1, "%2x", &t) == 1) {
                name[o++] = (char)t;
                i += 3;
            } else {
                break;
            }
        } else {
            name[o++] = buf[i++];
        }
    }
    /* trim trailing spaces */
    while (o && name[o - 1] == ' ') o--;
    name[o] = '\0';
}

int
xsynth_voice_read_patch(FILE *file, xsynth_patch_t *patch, unsigned long bank,
                        unsigned long program)
{
    int format, i;
    char buf[256], buf2[90];
    xsynth_patch_t tmp;

    do {
        if (!fgets(buf, 256, file)) return 0;
    } while (is_comment(buf));

    if (sscanf(buf, " xsynth-dssi patch format %d begin", &format) != 1 ||
        format < 0 || format > 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " name %90s", buf2) != 1) return 0;
    parse_name(buf2, tmp.name);

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " osc1 %f %d %f", &tmp.osc1_pitch, &i,
               &tmp.osc1_pulsewidth) != 3)
        return 0;
    tmp.osc1_waveform = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " osc2 %f %d %f", &tmp.osc2_pitch, &i,
               &tmp.osc2_pulsewidth) != 3)
        return 0;
    tmp.osc2_waveform = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " sync %d", &i) != 1)
        return 0;
    tmp.osc_sync = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " balance %f", &tmp.osc_balance) != 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " lfo %f %d %f %f", &tmp.lfo_frequency, &i,
               &tmp.lfo_amount_o, &tmp.lfo_amount_f) != 4)
        return 0;
    tmp.lfo_waveform = (unsigned char)i;

    if (format == 1) {

        if (!fgets(buf, 256, file)) return 0;
        if (sscanf(buf, " eg1 %f %f %f %f %f %f %f",
                   &tmp.eg1_attack_time, &tmp.eg1_decay_time,
                   &tmp.eg1_sustain_level, &tmp.eg1_release_time,
                   &tmp.eg1_vel_sens, &tmp.eg1_amount_o,
                   &tmp.eg1_amount_f) != 7)
            return 0;

        if (!fgets(buf, 256, file)) return 0;
        if (sscanf(buf, " eg2 %f %f %f %f %f %f %f",
                   &tmp.eg2_attack_time, &tmp.eg2_decay_time,
                   &tmp.eg2_sustain_level, &tmp.eg2_release_time,
                   &tmp.eg2_vel_sens, &tmp.eg2_amount_o,
                   &tmp.eg2_amount_f) != 7)
            return 0;

    } else {

        if (!fgets(buf, 256, file)) return 0;
        if (sscanf(buf, " eg1 %f %f %f %f %f %f",
                   &tmp.eg1_attack_time, &tmp.eg1_decay_time,
                   &tmp.eg1_sustain_level, &tmp.eg1_release_time,
                   &tmp.eg1_amount_o, &tmp.eg1_amount_f) != 6)
            return 0;

        if (!fgets(buf, 256, file)) return 0;
        if (sscanf(buf, " eg2 %f %f %f %f %f %f",
                   &tmp.eg2_attack_time, &tmp.eg2_decay_time,
                   &tmp.eg2_sustain_level, &tmp.eg2_release_time,
                   &tmp.eg2_amount_o, &tmp.eg2_amount_f) != 6)
            return 0;

        tmp.eg1_vel_sens = 0.0f;
        tmp.eg2_vel_sens = 0.0f;
    }

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " vcf %f %f %d", &tmp.vcf_cutoff, &tmp.vcf_qres, &i) != 3)
        return 0;
    tmp.vcf_mode = (unsigned char)i;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " glide %f", &tmp.glide_time) != 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " volume %f", &tmp.volume) != 1)
        return 0;

    if (!fgets(buf, 256, file)) return 0;
    if (sscanf(buf, " xsynth-dssi patch %3s", buf2) != 1) return 0;
    if (strcmp(buf2, "end")) return 0;

    memcpy(patch, &tmp, sizeof(xsynth_patch_t));

    return 1;  /* -FIX- error handling yet to be implemented */
}

/*
 * xsynth_data_load
 */
void
xsynth_data_load(const char *filename)
{
    FILE *fh;
    int count = 0;
    xsynth_patch_t patch;

    printf("attempting to load '%s'\n", filename);

    if ((fh = fopen(filename, "rb")) == NULL) {
        printf("load error: could not open file\n");
        return;
    }

    while (xsynth_voice_read_patch(fh, &patch, 0, count)) {
        xsynth_voice_write_patch_as_c(stdout, &patch);
        count++;
    }
    fclose(fh);

    if (!count) {
        printf("load error: no patches recognized\n");
        return;
    }
    printf("loaded %d patches\n", count);
    return;
}

int
main(int argc, char *argv[])
{
    xsynth_data_load(argv[1]);

    return 0;
}

