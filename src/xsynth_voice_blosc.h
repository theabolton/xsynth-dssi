/* Xsynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009 Sean Bolton.
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

/* Ah, the gentle tedium of moving all possible conditionals outside
 * the inner loops!
 *
 * This file gets included four times from xsynth_voice_render.c, to
 * define each of the four types of oscillator functions:
 * - single1 (VCO 1 without hard sync)
 * - master  (VCO 1 as hard sync master)
 * - single2 (VCO 2 without hard sync)
 * - slave   (VCO 2 as hard sync slave)
 *
 * As a example of how the BLOSC_THIS macro works, if BLOSC_SINGLE1 is
 * defined when this file is included, then:
 *     BLOSC_THIS(sine, ...)
 * gets replaced with:
 *     blosc_single1sine(...)
 */

#ifdef BLOSC_SINGLE1
/* #define BLOSC_THIS(x, ...) blosc_single1##x(__VA_ARGS__) */
#define BLOSC_THIS(x, args...) blosc_single1##x(args)
#define BLOSC_W_TABLE 0
#endif
#ifdef BLOSC_MASTER
#define BLOSC_THIS(x, args...) blosc_master##x(args)
#define BLOSC_W_TABLE 0
#endif
#ifdef BLOSC_SINGLE2
#define BLOSC_THIS(x, args...) blosc_single2##x(args)
#define BLOSC_W_TABLE 1
#endif
#ifdef BLOSC_SLAVE
#define BLOSC_THIS(x, args...) blosc_slave##x(args)
#define BLOSC_W_TABLE 1
#endif

/* ==== blosc_*sine functions ==== */

/* static inline */ void
#if BLOSC_W_TABLE
BLOSC_THIS(sine, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float *wp)
#else
BLOSC_THIS(sine, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float w)
#endif
{
    unsigned long sample;
    float pos = osc->pos;
#if BLOSC_W_TABLE
    float w;
#endif
    float frac;
    int   i;

    if (osc->last_waveform != osc->waveform) {
        pos = 0.0f;
        /* if we valued alias-free startup over low startup time, we could do:
         *   pos -= w;
         *   blosc_place_slope_dd(voice->osc_audio, index, 0.0f, w, gain * 0.5f * M_2PI_F); */
        osc->last_waveform = osc->waveform;
    }

    for (sample = 0; sample < sample_count; sample++) {

#if BLOSC_W_TABLE
        w = wp[sample];
#endif
        pos += w;

#ifdef BLOSC_SLAVE
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            float out, slope;
            pos = eof_offset;

            /* calculate amplitude and slope approaching reset point */
            if (pos_at_reset >= 1.0f) {
                pos_at_reset -= 1.0f;
            }
            frac = pos_at_reset * WAVE_POINTS;
            i = lrintf(frac - 0.5f);
            frac -= (float)i;
            i &= (WAVE_POINTS - 1);
            out = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * frac;
            i = (i + WAVE_POINTS / 4) & (WAVE_POINTS - 1);
            slope = sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * frac;
            /* now place reset DDs */
            blosc_place_slope_dd(voice->osc_audio, index, pos, w, gain * M_2PI_F * (0.5f - slope));
            blosc_place_step_dd(voice->osc_audio, index, pos, w, gain * (/* 0.0f */ - out));
        } else
#endif /* slave */
        if (pos >= 1.0f) {
            pos -= 1.0f;
#ifdef BLOSC_MASTER
            voice->osc_sync[sample] = pos / w;
        } else {
            voice->osc_sync[sample] = -1.0f;
#endif /* master */
        }

        frac = pos * WAVE_POINTS;
        i = lrintf(frac - 0.5f);
        frac -= (float)i;
        voice->osc_audio[index + DD_SAMPLE_DELAY] += gain *
            (sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * frac);

        index++;
    }

    osc->pos = pos;
}

/* ==== blosc_*tri functions ==== */

/* static inline */ void
#if BLOSC_W_TABLE
BLOSC_THIS(tri, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float *wp)
#else
BLOSC_THIS(tri, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float w)
#endif
{
    unsigned long sample;
#if BLOSC_W_TABLE
    float w;
#endif
    int   bp_high = osc->bp_high;
    float pos = osc->pos,
          pw, out, slope_delta;

    if (osc->waveform == 1) {                                 /* triangle */

        pw = 0.5f;
        slope_delta = gain * 4.0f;

    } else {                                   /* variable-slope triangle */

        pw = osc->pw;
#if BLOSC_W_TABLE
        w = *wp;
#endif
        if (pw < w) pw = w;  /* w is sample phase width */
        else if (pw > 1.0f - w) pw = 1.0f - w;
        slope_delta = gain * (1.0f / pw + 1.0f / (1.0f - pw));
    }

    if (osc->last_waveform != osc->waveform) {

        if (osc->waveform == 1) {                                 /* triangle */
            pos = 0.25f;
        } else {                                   /* variable-slope triangle */
            pos = 0.5f * pw;
        }
        /* if we valued alias-free startup over low startup time, we could do:
         *   pos -= w;
         *   blosc_place_slope_dd(voice->osc_audio, index, 0.0f, w, gain * 1.0f / *pw); */
        bp_high = 1;

        osc->last_waveform = osc->waveform;
    }

    for (sample = 0; sample < sample_count; sample++) {

#if BLOSC_W_TABLE
        w = wp[sample];
#endif
        pos += w;

#ifdef BLOSC_SLAVE
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DDs that may have occurred in subsample before reset */
            if (bp_high) {
                out = -0.5f + pos_at_reset / pw;
                if (pos_at_reset >= pw) {
                    out = 0.5f - (pos_at_reset - pw) / (1.0f - pw);
                    blosc_place_slope_dd(voice->osc_audio, index, pos_at_reset - pw + eof_offset, w, -slope_delta);
                    bp_high = 0;
                }
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    out = -0.5f + pos_at_reset / pw;
                    blosc_place_slope_dd(voice->osc_audio, index, pos_at_reset + eof_offset, w, slope_delta);
                    bp_high = 1;
                }
            } else {
                out = 0.5f - (pos_at_reset - pw) / (1.0f - pw);
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    out = -0.5f + pos_at_reset / pw;
                    blosc_place_slope_dd(voice->osc_audio, index, pos_at_reset + eof_offset, w, slope_delta);
                    bp_high = 1;
                }
                if (bp_high && pos_at_reset >= pw) {
                    out = 0.5f - (pos_at_reset - pw) / (1.0f - pw);
                    blosc_place_slope_dd(voice->osc_audio, index, pos_at_reset - pw + eof_offset, w, -slope_delta);
                    bp_high = 0;
                }
            }

            /* now place reset DDs */
            if (!bp_high)
                blosc_place_slope_dd(voice->osc_audio, index, pos, w, slope_delta);
            blosc_place_step_dd(voice->osc_audio, index, pos, w, gain * (-0.5f - out));
            out = -0.5f + pos / pw;
            bp_high = 1;
            if (pos >= pw) {
                out = 0.5f - (pos - pw) / (1.0f - pw);
                blosc_place_slope_dd(voice->osc_audio, index, pos - pw, w, -slope_delta);
                bp_high = 0;
            }
        } else
#endif /* slave */
        if (bp_high) {
            out = -0.5f + pos / pw;
            if (pos >= pw) {
                out = 0.5f - (pos - pw) / (1.0f - pw);
                blosc_place_slope_dd(voice->osc_audio, index, pos - pw, w, -slope_delta);
                bp_high = 0;
            }
            if (pos >= 1.0f) {
                pos -= 1.0f;
#ifdef BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                out = -0.5f + pos / pw;
                blosc_place_slope_dd(voice->osc_audio, index, pos, w, slope_delta);
                bp_high = 1;
#ifdef BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
        } else {
            out = 0.5f - (pos - pw) / (1.0f - pw);
            if (pos >= 1.0f) {
                pos -= 1.0f;
#ifdef BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                out = -0.5f + pos / pw;
                blosc_place_slope_dd(voice->osc_audio, index, pos, w, slope_delta);
                bp_high = 1;
#ifdef BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
            if (bp_high && pos >= pw) {
                out = 0.5f - (pos - pw) / (1.0f - pw);
                blosc_place_slope_dd(voice->osc_audio, index, pos - pw, w, -slope_delta);
                bp_high = 0;
            }
        }
        voice->osc_audio[index + DD_SAMPLE_DELAY] += gain * out;

        index++;
    }

    osc->pos = pos;
    osc->bp_high = bp_high;
}

/* ==== blosc_*sawup functions ==== */

/* static inline */ void
#if BLOSC_W_TABLE
BLOSC_THIS(sawup, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float *wp)
#else
BLOSC_THIS(sawup, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float w)
#endif
{
    unsigned long sample;
#if BLOSC_W_TABLE
    float w;
#endif
    float pos = osc->pos;

    if (osc->last_waveform != osc->waveform) {

        /* this would be the cleanest startup:
         *   pos = 0.5f - w;
         *   blosc_place_slope_dd(voice->osc_audio, index, 0.0f, w, 1.0f);
         * but we have to match the phase of the original Xsynth code: */
        pos = 0.0f;

        osc->last_waveform = osc->waveform;
    }

    for (sample = 0; sample < sample_count; sample++) {

#if BLOSC_W_TABLE
        w = wp[sample];
#endif
        pos += w;

#ifdef BLOSC_SLAVE
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DD that may have occurred in subsample before reset */
            if (pos_at_reset >= 1.0f) {
                pos_at_reset -= 1.0f;
                blosc_place_step_dd(voice->osc_audio, index, pos_at_reset + eof_offset, w, -gain);
            }

            /* now place reset DD */
            blosc_place_step_dd(voice->osc_audio, index, pos, w, -gain * pos_at_reset);
        } else
#endif /* slave */
        if (pos >= 1.0f) {
            pos -= 1.0f;
#ifdef BLOSC_MASTER
            voice->osc_sync[sample] = pos / w;
#endif /* master */
            blosc_place_step_dd(voice->osc_audio, index, pos, w, -gain);
#ifdef BLOSC_MASTER
        } else {
            voice->osc_sync[sample] = -1.0f;
#endif /* master */
        }
        voice->osc_audio[index + DD_SAMPLE_DELAY] += gain * (-0.5f + pos);

        index++;
    }

    osc->pos = pos;
}

/* ==== blosc_*sawdown functions ==== */

/* static inline */ void
#if BLOSC_W_TABLE
BLOSC_THIS(sawdown, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float *wp)
#else
BLOSC_THIS(sawdown, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float w)
#endif
{
    unsigned long sample;
#if BLOSC_W_TABLE
    float w;
#endif
    float pos = osc->pos;

    if (osc->last_waveform != osc->waveform) {

        /* this would be the cleanest startup:
         *   pos = 0.5f - w;
         *   blosc_place_slope_dd(voice->osc_audio, index, 0.0f, w, -1.0f);
         * but we have to match the phase of the original Xsynth code: */
        pos = 0.0f;

        osc->last_waveform = osc->waveform;
    }

    for (sample = 0; sample < sample_count; sample++) {

#if BLOSC_W_TABLE
        w = wp[sample];
#endif
        pos += w;

#ifdef BLOSC_SLAVE
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DD that may have occurred in subsample before reset */
            if (pos_at_reset >= 1.0f) {
                pos_at_reset -= 1.0f;
                blosc_place_step_dd(voice->osc_audio, index, pos_at_reset + eof_offset, w, gain);
            }

            /* now place reset DD */
            blosc_place_step_dd(voice->osc_audio, index, pos, w, gain * pos_at_reset);
        } else
#endif /* slave */
        if (pos >= 1.0f) {
            pos -= 1.0f;
#ifdef BLOSC_MASTER
            voice->osc_sync[sample] = pos / w;
#endif /* master */
            blosc_place_step_dd(voice->osc_audio, index, pos, w, gain);
#ifdef BLOSC_MASTER
        } else {
            voice->osc_sync[sample] = -1.0f;
#endif /* master */
        }
        voice->osc_audio[index + DD_SAMPLE_DELAY] += gain * (0.5f - pos);

        index++;
    }

    osc->pos = pos;
}

/* ==== blosc_*rect functions ==== */

/* static inline */ void
#if BLOSC_W_TABLE
BLOSC_THIS(rect, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float *wp)
#else
BLOSC_THIS(rect, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float w)
#endif
{
    unsigned long sample;
#if BLOSC_W_TABLE
    float w;
#endif
    int   bp_high = osc->bp_high;
    float pos = osc->pos,
          pw,
          halfgain = gain * 0.5f,
          out = (bp_high ? halfgain : -halfgain);

    if (osc->waveform == 4) {                                  /* square wave */

        pw = 0.5f;

    } else {                                          /* variable-width pulse */

        pw = osc->pw;
#if BLOSC_W_TABLE
        w = *wp;
#endif
        if (pw < w) pw = w;  /* w is sample phase width */
        else if (pw > 1.0f - w) pw = 1.0f - w;
    }

    if (osc->last_waveform != osc->waveform) {

        pos = 0.0f;
        /* for waveform 5, variable-width pulse, we could do DC compensation with:
         *     out = halfgain * (1.0f - pw);
         * but that doesn't work well with highly modulated hard sync.  Instead,
         * we keep things in the range [-0.5f, 0.5f]. */
        out = halfgain;
        bp_high = 1;
        /* if we valued alias-free startup over low startup time, we could do:
         *   pos -= w;
         *   blosc_place_step_dd(voice->osc_audio, index, 0.0f, w, halfgain); */

        osc->last_waveform = osc->waveform;
    }

    for (sample = 0; sample < sample_count; sample++) {

#if BLOSC_W_TABLE
        w = wp[sample];
#endif
        pos += w;

#ifdef BLOSC_SLAVE
        if (voice->osc_sync[sample] >= 0.0f) { /* sync to master */

            float eof_offset = voice->osc_sync[sample] * w;
            float pos_at_reset = pos - eof_offset;
            pos = eof_offset;

            /* place any DDs that may have occurred in subsample before reset */
            if (bp_high) {
                if (pos_at_reset >= pw) {
                    blosc_place_step_dd(voice->osc_audio, index, pos_at_reset - pw + eof_offset, w, -gain);
                    bp_high = 0;
                    out = -halfgain;
                }
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    blosc_place_step_dd(voice->osc_audio, index, pos_at_reset + eof_offset, w, gain);
                    bp_high = 1;
                    out = halfgain;
                }
            } else {
                if (pos_at_reset >= 1.0f) {
                    pos_at_reset -= 1.0f;
                    blosc_place_step_dd(voice->osc_audio, index, pos_at_reset + eof_offset, w, gain);
                    bp_high = 1;
                    out = halfgain;
                }
                if (bp_high && pos_at_reset >= pw) {
                    blosc_place_step_dd(voice->osc_audio, index, pos_at_reset - pw + eof_offset, w, -gain);
                    bp_high = 0;
                    out = -halfgain;
                }
            }

            /* now place reset DD */
            if (!bp_high) {
                blosc_place_step_dd(voice->osc_audio, index, pos, w, gain);
                bp_high = 1;
                out = halfgain;
            }
            if (pos >= pw) {
                blosc_place_step_dd(voice->osc_audio, index, pos - pw, w, -gain);
                bp_high = 0;
                out = -halfgain;
            }
        } else
#endif /* slave */
        if (bp_high) {
            if (pos >= pw) {
                blosc_place_step_dd(voice->osc_audio, index, pos - pw, w, -gain);
                bp_high = 0;
                out = -halfgain;
            }
            if (pos >= 1.0f) {
                pos -= 1.0f;
#ifdef BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                blosc_place_step_dd(voice->osc_audio, index, pos, w, gain);
                bp_high = 1;
                out = halfgain;
#ifdef BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
        } else {
            if (pos >= 1.0f) {
                pos -= 1.0f;
#ifdef BLOSC_MASTER
                voice->osc_sync[sample] = pos / w;
#endif /* master */
                blosc_place_step_dd(voice->osc_audio, index, pos, w, gain);
                bp_high = 1;
                out = halfgain;
#ifdef BLOSC_MASTER
            } else {
                voice->osc_sync[sample] = -1.0f;
#endif /* master */
            }
            if (bp_high && pos >= pw) {
                blosc_place_step_dd(voice->osc_audio, index, pos - pw, w, -gain);
                bp_high = 0;
                out = -halfgain;
            }
        }
        voice->osc_audio[index + DD_SAMPLE_DELAY] += out;

        index++;
    }

    osc->pos = pos;
    osc->bp_high = bp_high;
}

/* ==== blosc_* dispatch function ==== */

static inline void
#if BLOSC_W_TABLE
BLOSC_THIS(, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float *w)
#else
BLOSC_THIS(, unsigned long sample_count, xsynth_voice_t *voice,
           struct blosc *osc, int index, float gain, float w)
#endif
{
    switch (osc->waveform) {
      default:
      case 0:                                                    /* sine wave */
        BLOSC_THIS(sine, sample_count, voice, osc, index, gain, w);
        break;
      case 1:                                                /* triangle wave */
      case 6:                                 /* variable-slope triangle wave */
        BLOSC_THIS(tri, sample_count, voice, osc, index, gain, w);
        break;
      case 2:                                             /* up sawtooth wave */
        BLOSC_THIS(sawup, sample_count, voice, osc, index, gain, w);
        break;
      case 3:                                           /* down sawtooth wave */
        BLOSC_THIS(sawdown, sample_count, voice, osc, index, gain, w);
        break;
      case 4:                                                  /* square wave */
      case 5:                                                   /* pulse wave */
        BLOSC_THIS(rect, sample_count, voice, osc, index, gain, w);
        break;
    }
}

#undef BLOSC_THIS
#undef BLOSC_W_TABLE

