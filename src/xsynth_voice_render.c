/* Xsynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Much of this file comes from Steve Brookes' Xsynth,
 * copyright (C) 1999 S. J. Brookes.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
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

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <math.h>

#include <ladspa.h>

#include "xsynth.h"
#include "xsynth_synth.h"
#include "xsynth_voice.h"

 /********************************************************************
 *                                                                   *
 * Xsynth-DSSI started out as a DSSI demonstration plugin, and       *
 * originally this file was just Steve Brookes' Xsynth code with as  *
 * little modification as was necessary to interface it with my DSSI *
 * plugin code.  If you'd like to see that simpler version, check    *
 * out the included file src/xsynth_voice_render-original.c.         *
 *                                                                   *
 * And Steve, wherever you are -- thanks.                            *
 *                                                                   *
 ********************************************************************/

#define VCF_FREQ_MAX  (0.825f)    /* filter only stable to this frequency */

float        xsynth_pitch[128];

static float sine_wave[WAVE_POINTS+1],
             triangle_wave[WAVE_POINTS+1];

#define pitch_ref_pitch 440.0
#define pitch_ref_note 69

#define volume_to_amplitude_scale 128

static float volume_to_amplitude_table[4 + volume_to_amplitude_scale + 2];

static float velocity_to_attenuation[128];

static float qdB_to_amplitude_table[4 + 256 + 0];

void
xsynth_init_tables(void)
{
    int i, qn, tqn;
    float pexp;
    float volume, volume_exponent;
    float ol, amp;

    /* oscillator waveforms */
    for (i = 0; i <= WAVE_POINTS; ++i) {
        sine_wave[i] = sin(2.0f * M_PI * (float)i / WAVE_POINTS);
    }

    qn = WAVE_POINTS / 4;
    tqn = 3 * WAVE_POINTS / 4;

    for (i = 0; i <= WAVE_POINTS; ++i) {
        if (i < qn)
            triangle_wave[i] = (float)i / (float)qn;
        else if (i < tqn)
            triangle_wave[i] = 1.0f - 2.0f * (float)(i - qn) / (float)(tqn - qn);
        else
            triangle_wave[i] = (float)(i - tqn) / (float)(WAVE_POINTS - tqn) - 1.0f;
    }

    /* MIDI note to pitch */
    for (i = 0; i < 128; ++i) {
        pexp = (float)(i - pitch_ref_note) / 12.0f;
        xsynth_pitch[i] = pitch_ref_pitch * pow(2.0f, pexp);
    }

    /* volume to amplitude
     *
     * This generates a curve which is:
     *  volume_to_amplitude_table[128 + 4] = 1.0       =   0dB
     *  volume_to_amplitude_table[64 + 4]  = 0.316...  = -10dB
     *  volume_to_amplitude_table[32 + 4]  = 0.1       = -20dB
     *  volume_to_amplitude_table[16 + 4]  = 0.0316... = -30dB
     *   etc.
     */
    volume_exponent = 1.0f / (2.0f * log10f(2.0f));
    for(i = 0; i <= volume_to_amplitude_scale; i++) {
        volume = (float)i / (float)volume_to_amplitude_scale;
        volume_to_amplitude_table[i + 4] = powf(volume, volume_exponent);
    }
    volume_to_amplitude_table[ -1 + 4] = 0.0f;
    volume_to_amplitude_table[129 + 4] = 1.0f;

    /* velocity to attenuation
     *
     * Creates the velocity to attenuation lookup table, for converting
     * velocities [1, 127] to full-velocity-sensitivity attenuation in
     * quarter decibels.  Modeled after my TX-7's velocity response.*/
    velocity_to_attenuation[0] = 253.9999f;
    for (i = 1; i < 127; i++) {
        if (i >= 10) {
            ol = (powf(((float)i / 127.0f), 0.32f) - 1.0f) * 100.0f;
            amp = powf(2.0f, ol / 8.0f);
        } else {
            ol = (powf(((float)10 / 127.0f), 0.32f) - 1.0f) * 100.0f;
            amp = powf(2.0f, ol / 8.0f) * (float)i / 10.0f;
        }
        velocity_to_attenuation[i] = log10f(amp) * -80.0f;
    }
    velocity_to_attenuation[127] = 0.0f;

    /* quarter-decibel attenuation to amplitude */
    qdB_to_amplitude_table[3] = 1.0f;
    for (i = 0; i <= 255; i++) {
        qdB_to_amplitude_table[i + 4] = powf(10.0f, (float)i / -80.0f);
    }
}

static inline float
volume(float level)
{
    unsigned char segment;
    float fract;

    level *= (float)volume_to_amplitude_scale;
    segment = lrintf(level - 0.5);
    fract = level - (float)segment;

    return volume_to_amplitude_table[segment + 4] + fract *
               (volume_to_amplitude_table[segment + 5] -
                volume_to_amplitude_table[segment + 4]);
}

static inline float
qdB_to_amplitude(float qdB)
{
    int i = lrintf(qdB - 0.5f);
    float f = qdB - (float)i;
    return qdB_to_amplitude_table[i + 4] + f *
           (qdB_to_amplitude_table[i + 5] -
            qdB_to_amplitude_table[i + 4]);
}

static inline float
oscillator(float *pos, float omega, float deltat,
           unsigned char waveform, float pw, unsigned char *sync)
{
    float wpos, f;
    unsigned char i;

    *pos += deltat * omega;

    if (*pos >= 1.0f) {
        *pos -= 1.0f;
        *sync = 1;
    }

    switch (waveform) {
      default:
      case 0:                                                    /* sine wave */
        wpos = *pos * WAVE_POINTS;
        i = (unsigned char)lrintf(floorf(wpos));
        f = wpos - (float)i;
        return (sine_wave[i] + (sine_wave[i + 1] - sine_wave[i]) * f);

      case 1:                                                /* triangle wave */
        wpos = *pos * WAVE_POINTS;
        i = (unsigned char)lrintf(floorf(wpos));
        f = wpos - (float)i;
        return (triangle_wave[i] + (triangle_wave[i + 1] - triangle_wave[i]) * f);

      case 2:                                             /* up sawtooth wave */
        return (*pos * 2.0f - 1.0f);

      case 3:                                           /* down sawtooth wave */
        return (1.0f - *pos * 2.0f);

      case 4:                                                  /* square wave */
        return ((*pos < 0.5f) ? 1.0f : -1.0f);

      case 5:                                                   /* pulse wave */
        return ((*pos < pw) ? 1.0f : -1.0f);

      case 6:                                 /* variable-slope triangle wave */
        if (*pos < pw)
            return -1.0f + (*pos / pw) * 2.0f;
        else
            return 1.0f - ((*pos - pw) / (1.0f - pw)) * 2.0f;
    }
}

/*
 * xsynth_voice_render
 *
 * generate the actual sound data for this voice
 */
void
xsynth_voice_render(xsynth_synth_t *synth, xsynth_voice_t *voice,
                    LADSPA_Data *out, unsigned long sample_count,
                    int do_control_update)
{
    unsigned long sample;

    /* state variables saved in voice */

    float         lfo_pos    = voice->lfo_pos,
                  eg1        = voice->eg1,
                  eg2        = voice->eg2,
                  osc1_pos   = voice->osc1_pos,
                  osc2_pos   = voice->osc2_pos,
                  delay1     = voice->delay1,
                  delay2     = voice->delay2,
                  delay3     = voice->delay3,
                  delay4     = voice->delay4;
    unsigned char eg1_phase  = voice->eg1_phase,
                  eg2_phase  = voice->eg2_phase;

    /* temporary variables used in calculating voice */

    float fund_pitch;
    float deltat = 1.0f / (float)synth->sample_rate;
    float freq, freqkey, freqeg1, freqeg2;
    float lfo, osc1, deltat2, omega2_t, osc2;
    float input, freqcut, highpass, output;
    unsigned char sync_flag1 = 0, sync_flag2 = 0, sync_flag3 = 0;

    /* set up synthesis variables from patch */
    float         omega1, omega2;
    unsigned char osc1_waveform = lrintf(*(synth->osc1_waveform));
    float         osc1_pw = *(synth->osc1_pulsewidth);
    unsigned char osc2_waveform = lrintf(*(synth->osc2_waveform));
    float         osc2_pw = *(synth->osc2_pulsewidth);
    unsigned char osc_sync = (*(synth->osc_sync) > 0.0001f);
    float         omega3 = *(synth->lfo_frequency);
    unsigned char lfo_waveform = lrintf(*(synth->lfo_waveform));
    float         lfo_amount_o = *(synth->lfo_amount_o);
    float         lfo_amount_f = *(synth->lfo_amount_f);
    float         eg1_amp = qdB_to_amplitude(velocity_to_attenuation[voice->velocity] *
                                             *(synth->eg1_vel_sens));
    float         eg1_rate_level[3], eg1_one_rate[3];
    float         eg1_amount_o = *(synth->eg1_amount_o);
    float         eg2_amp = qdB_to_amplitude(velocity_to_attenuation[voice->velocity] *
                                             *(synth->eg2_vel_sens));
    float         eg2_rate_level[3], eg2_one_rate[3];
    float         eg2_amount_o = *(synth->eg2_amount_o);
    float         qres = 0.005 + (1.995f - *(synth->vcf_qres)) * voice->pressure;
    unsigned char pole4 = (*(synth->vcf_4pole) > 0.0001f);
    float         balance1 = 1.0f - *(synth->osc_balance);
    float         balance2 = *(synth->osc_balance);
    float         vol_out = volume(*(synth->volume));

    fund_pitch = *(synth->glide_time) * voice->target_pitch +
                 (1.0f - *(synth->glide_time)) * voice->prev_pitch;    /* portamento */

    voice->prev_pitch = fund_pitch;                                 /* save pitch for next time */
    fund_pitch *= synth->pitch_bend;                                /* modify pitch after portamento */
    
    omega1 = *(synth->osc1_pitch) * fund_pitch;
    omega2 = *(synth->osc2_pitch) * fund_pitch;

    eg1_rate_level[0] = *(synth->eg1_attack_time) * eg1_amp;  /* eg1_attack_time * 1.0f * eg1_amp */
    eg1_one_rate[0] = 1.0f - *(synth->eg1_attack_time);
    eg1_rate_level[1] = *(synth->eg1_decay_time) * *(synth->eg1_sustain_level) * eg1_amp;
    eg1_one_rate[1] = 1.0f - *(synth->eg1_decay_time);
    eg1_rate_level[2] = 0.0f;                                 /* eg1_release_time * 0.0f * eg1_amp */
    eg1_one_rate[2] = 1.0f - *(synth->eg1_release_time);
    eg2_rate_level[0] = *(synth->eg2_attack_time) * eg2_amp;
    eg2_one_rate[0] = 1.0f - *(synth->eg2_attack_time);
    eg2_rate_level[1] = *(synth->eg2_decay_time) * *(synth->eg2_sustain_level) * eg2_amp;
    eg2_one_rate[1] = 1.0f - *(synth->eg2_decay_time);
    eg2_rate_level[2] = 0.0f;
    eg2_one_rate[2] = 1.0f - *(synth->eg2_release_time);

    eg1_amp *= 0.99f;  /* Xsynth's original eg phase 1 to 2 transition check was:  */
    eg2_amp *= 0.99f;  /*    if (!eg1_phase && eg1 > 0.99f) eg1_phase = 1;         */

    freq = 2.0f * M_PI / (float)synth->sample_rate * fund_pitch * synth->mod_wheel;
    freqkey = freq * *(synth->vcf_cutoff);
    freqeg1 = freq * *(synth->eg1_amount_f);
    freqeg2 = freq * *(synth->eg2_amount_f);

    /* calculate voice */

    for (sample = 0; sample < sample_count; sample++) {

        /* --- LFO section */

        lfo = oscillator(&lfo_pos, omega3, deltat, lfo_waveform, 0.25f, &sync_flag3);

        /* --- EG1 section */

        eg1 = eg1_rate_level[eg1_phase] + eg1_one_rate[eg1_phase] * eg1;

        if (!eg1_phase && eg1 > eg1_amp) eg1_phase = 1;  /* flip from attack to decay */

        /* --- EG2 section */

        eg2 = eg2_rate_level[eg2_phase] + eg2_one_rate[eg2_phase] * eg2;

        if (!eg2_phase && eg2 > eg2_amp) eg2_phase = 1;  /* flip from attack to decay */

        /* --- VCO 1 section */

        osc1 = oscillator(&osc1_pos, omega1, deltat, osc1_waveform, osc1_pw, &sync_flag1);

        /* --- oscillator sync control */

        if (osc_sync & sync_flag1) {
            sync_flag1 = 0;
            osc2_pos = 0.0f;
            deltat2 = osc1_pos / omega1;
        } else {
            deltat2 = deltat;
        }

        /* --- VCO 2 section */

        omega2_t = omega2 *
                   (1.0f + eg1 * eg1_amount_o) *
                   (1.0f + eg2 * eg2_amount_o) *
                   (1.0f + lfo * lfo_amount_o);

        osc2 = oscillator(&osc2_pos, omega2_t, deltat2, osc2_waveform, osc2_pw, &sync_flag2);

        /* --- cross modulation */

        /* --- mixer section */

        input = balance1 * osc1 + balance2 * osc2;

        /* --- VCF section - Hal Chamberlin's state variable filter */

        freqcut = (freqkey + freqeg1 * eg1 + freqeg2 * eg2) * (1.0f + lfo * lfo_amount_f);

        if (freqcut > VCF_FREQ_MAX) freqcut = VCF_FREQ_MAX;

        delay2 = delay2 + freqcut * delay1;             /* delay2/4 = lowpass output */
        highpass = input - delay2 - qres * delay1;
        delay1 = freqcut * highpass + delay1;           /* delay1/3 = bandpass output */
        output = delay2;

        if (pole4) {  /* above gives 12db per octave, this gives 24db per octave */
            delay4 = delay4 + freqcut * delay3;
            highpass = output - delay4 - qres * delay3;
            delay3 = freqcut * highpass + delay3;
            output = delay4;
        }

        /* --- VCA section */

        output *= eg1 * vol_out;

        /* mix voice output into output buffer */
        out[sample] += output;

        /* update runtime parameters for next sample */
    }

    if (do_control_update) {
        /* do those things should be done only once per control-calculation
         * interval ("nugget"), such as voice check-for-dead, pitch envelope
         * calculations, volume envelope phase transition checks, etc. */

        /* check if we've decayed to nothing, turn off voice if so */
        if (eg1_phase == 2 && eg1 < 1.0e-5f) {  /* sound has completed its release phase */

            XDB_MESSAGE(XDB_NOTE, " xsynth_voice_render check for dead: killing note id %d\n", voice->note_id);
            xsynth_voice_off(voice);
            return; /* we're dead now, so return */
        }
    }

    /* save things for next time around */

    /* already saved prev_pitch above */
    voice->lfo_pos    = lfo_pos;
    voice->eg1        = eg1;
    voice->eg1_phase  = eg1_phase;
    voice->eg2        = eg2;
    voice->eg2_phase  = eg2_phase;
    voice->osc1_pos   = osc1_pos;
    voice->osc2_pos   = osc2_pos;
    voice->delay1     = delay1;
    voice->delay2     = delay2;
    voice->delay3     = delay3;
    voice->delay4     = delay4;
}

