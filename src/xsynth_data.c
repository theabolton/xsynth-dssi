/* Xsynth DSSI software synthesizer plugin and GUI
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dssi.h"

#include "xsynth_voice.h"

xsynth_patch_t xsynth_init_voice = {
    "  <-->",
    1.0f, 0, 0.5f,                            /* osc1 */
    1.0f, 0, 0.5f,                            /* osc2 */
    0,                                        /* sync */
    0.5f,                                     /* balance */
    0.1f, 0, 0.0f, 0.0f,                      /* lfo */
    0.1f, 0.1f, 1.0f, 0.1f, 0.0f, 0.0f, 0.0f, /* eg1 */
    0.1f, 0.1f, 1.0f, 0.1f, 0.0f, 0.0f, 0.0f, /* eg2 */
    50.0f, 0.0f, 0,                           /* vcf */
    0.984375f,                                /* glide */
    0.5f                                      /* volume */
};

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
parse_name(const char *buf, char *name, int *inlen)
{
    int i = 0, o = 0;
    unsigned int t;

    while (buf[i] && o < 30) {
        if (buf[i] < 33 || buf[i] > 126) {
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

    if (inlen) *inlen = i;
}

int
xsynth_data_read_patch(FILE *file, xsynth_patch_t *patch)
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
    parse_name(buf2, tmp.name, NULL);

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

int
xsynth_data_decode_patches(const char *encoded, xsynth_patch_t *patches)
{
    int j, n, i0, i1, i2, i3;
    const char *ep = encoded;
    xsynth_patch_t *tmp, *pp;

    if (strncmp(ep, "Xp0 ", 4)) {
        /* fprintf(stderr, "bad header\n"); */
        return 0;  /* bad format */
    }
    ep += 4;

    tmp = (xsynth_patch_t *)malloc(32 * sizeof(xsynth_patch_t));
    if (!tmp)
        return 0;  /* out of memory */
    
    for (j = 0; j < 32; j++) {
        pp = &tmp[j];

        parse_name(ep, pp->name, &n);
        if (!n) {
            /* fprintf(stderr, "failed in name\n"); */
            break;
        }
        ep += n;

	if (sscanf(ep, " %f %d %f %f %d %f %d %f %f %d %f %f%n",
                   &pp->osc1_pitch, &i0, &pp->osc1_pulsewidth,
                   &pp->osc2_pitch, &i1, &pp->osc2_pulsewidth,
                   &i2, &pp->osc_balance, &pp->lfo_frequency,
                   &i3, &pp->lfo_amount_o, &pp->lfo_amount_f,
                   &n) != 12) {
            /* fprintf(stderr, "failed in oscs\n"); */
            break;
        }
        pp->osc1_waveform = (unsigned char)i0;
        pp->osc2_waveform = (unsigned char)i1;
        pp->osc_sync = (unsigned char)i2;
        pp->lfo_waveform = (unsigned char)i3;
        ep += n;

	if (sscanf(ep, " %f %f %f %f %f %f %f %f %f %f %f %f %f %f%n",
                   &pp->eg1_attack_time, &pp->eg1_decay_time,
                   &pp->eg1_sustain_level, &pp->eg1_release_time,
                   &pp->eg1_vel_sens, &pp->eg1_amount_o, &pp->eg1_amount_f,
                   &pp->eg2_attack_time, &pp->eg2_decay_time,
                   &pp->eg2_sustain_level, &pp->eg2_release_time,
                   &pp->eg2_vel_sens, &pp->eg2_amount_o, &pp->eg2_amount_f,
                   &n) != 14) {
            /* fprintf(stderr, "failed in egs\n"); */
            break;
        }
        ep += n;

	if (sscanf(ep, " %f %f %d %f %f%n",
                   &pp->vcf_cutoff, &pp->vcf_qres, &i0,
                   &pp->glide_time, &pp->volume,
                   &n) != 5) {
            /* fprintf(stderr, "failed in vcf+\n"); */
            break;
        }
        pp->vcf_mode = (unsigned char)i0;
        ep += n;

        while (*ep == ' ') ep++;
    }

    if (j != 32 || strcmp(ep, "end")) {
        /* fprintf(stderr, "decode failed, j = %d, *ep = 0x%02x\n", j, *ep); */
        free(tmp);
        return 0;  /* too few patches, or otherwise bad format */
    }

    memcpy(patches, tmp, 32 * sizeof(xsynth_patch_t));

    free(tmp);

    return 1;
}

