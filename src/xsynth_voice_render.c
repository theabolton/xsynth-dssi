/* Xsynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Much of this file comes from Steve Brookes' Xsynth,
 * copyright (C) 1999 S. J. Brookes.
 * Portions of this file come from Fons Adriaensen's VCO-plugins
 * and MCP-plugins, copyright (C) 2003 Fons Adriaensen.
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
 * Since that beginning, the following code has gained band-limited  *
 * oscillators, velocity sensitive envelopes, a more stable filter,  *
 * and some measure of optimization, so it's considerably more       *
 * complex.                                                          *
 *                                                                   *
 * Oh, and Steve, wherever you are -- thanks.                        *
 *                                                                   *
 ********************************************************************/

#define M_2PI_F (2.0f * (float)M_PI)
#define M_PI_F (float)M_PI

#define VCF_FREQ_MAX  (0.825f)    /* original filters only stable to this frequency */

static int   tables_initialized = 0;

float        xsynth_pitch[128];

#define WAVE_POINTS 1024          /* must be a power of two */

static float sine_wave[4 + WAVE_POINTS + 1],
             triangle_wave[4 + WAVE_POINTS + 1];

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

    if (tables_initialized)
        return;

    /* oscillator waveforms */
    for (i = 0; i <= WAVE_POINTS; ++i) {
        sine_wave[i + 4] = sinf(M_2PI_F * (float)i / (float)WAVE_POINTS) * 0.5f;
    }
    sine_wave[-1 + 4] = sine_wave[WAVE_POINTS - 1 + 4];  /* guard points both ends */

    qn = WAVE_POINTS / 4;
    tqn = 3 * WAVE_POINTS / 4;

    for (i = 0; i <= WAVE_POINTS; ++i) {
        if (i < qn)
            triangle_wave[i + 4] = (float)i / (float)qn;
        else if (i < tqn)
            triangle_wave[i + 4] = 1.0f - 2.0f * (float)(i - qn) / (float)(tqn - qn);
        else
            triangle_wave[i + 4] = (float)(i - tqn) / (float)(WAVE_POINTS - tqn) - 1.0f;
    }
    triangle_wave[-1 + 4] = triangle_wave[WAVE_POINTS - 1 + 4];

    /* MIDI note to pitch */
    for (i = 0; i < 128; ++i) {
        pexp = (float)(i - pitch_ref_note) / 12.0f;
        xsynth_pitch[i] = powf(2.0f, pexp);
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
    for (i = 0; i <= volume_to_amplitude_scale; i++) {
        volume = (float)i / (float)volume_to_amplitude_scale;
        volume_to_amplitude_table[i + 4] = powf(volume, volume_exponent) * 2.0f;
    }
    volume_to_amplitude_table[ -1 + 4] = 0.0f;
    volume_to_amplitude_table[129 + 4] = volume_to_amplitude_table[128 + 4];

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
    qdB_to_amplitude_table[-1 + 4] = 1.0f;
    for (i = 0; i <= 255; i++) {
        qdB_to_amplitude_table[i + 4] = powf(10.0f, (float)i / -80.0f);
    }

    tables_initialized = 1;
}

static inline float
volume(float level)
{
    unsigned char segment;
    float fract;

    level *= (float)volume_to_amplitude_scale;
    segment = lrintf(level - 0.5f);
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
oscillator(float *pos, float omega, float deltat, unsigned char waveform)
{
    float wpos, f;
    int i;

    *pos += deltat * omega;

    if (*pos >= 1.0f) {
        *pos -= 1.0f;
    }

    switch (waveform) {
      default:
      case 0:                                                    /* sine wave */
        wpos = *pos * WAVE_POINTS;
        i = lrintf(wpos - 0.5f);
        f = wpos - (float)i;
        return (sine_wave[i + 4] + (sine_wave[i + 5] - sine_wave[i + 4]) * f) * 2.0f;

      case 1:                                                /* triangle wave */
        wpos = *pos * WAVE_POINTS;
        i = lrintf(wpos - 0.5f);
        f = wpos - (float)i;
        return (triangle_wave[i + 4] + (triangle_wave[i + 5] - triangle_wave[i + 4]) * f);

      case 2:                                             /* up sawtooth wave */
        return (*pos * 2.0f - 1.0f);

      case 3:                                           /* down sawtooth wave */
        return (1.0f - *pos * 2.0f);

      case 4:                                                  /* square wave */
        return ((*pos < 0.5f) ? 1.0f : -1.0f);

      case 5:                                                   /* pulse wave */
        return ((*pos < 0.25f) ? 1.0f : -1.0f);
    }
}

static inline void
blosc_place_step_dd(float *buffer, int index, float phase, float w, float scale)
{
    float r;
    int i;

    r = MINBLEP_PHASES * phase / w;
    i = lrintf(r - 0.5f);
    r -= (float)i;
    i &= MINBLEP_PHASE_MASK;  /* port changes can cause i to be out-of-range */
    /* This would be better than the above, but more expensive:
     *  while (i < 0) {
     *    i += MINBLEP_PHASES;
     *    index++;
     *  }
     */

    while (i < MINBLEP_PHASES * STEP_DD_PULSE_LENGTH) {
        buffer[index] += scale * (step_dd_table[i].value + r * step_dd_table[i].delta);
        i += MINBLEP_PHASES;
        index++;
    }
}

static inline void
blosc_place_slope_dd(float *buffer, int index, float phase, float w, float slope_delta)
{
    float r;
    int i;

    r = MINBLEP_PHASES * phase / w;
    i = lrintf(r - 0.5f);
    r -= (float)i;
    i &= MINBLEP_PHASE_MASK;  /* port changes can cause i to be out-of-range */

    slope_delta *= w;

    while (i < MINBLEP_PHASES * SLOPE_DD_PULSE_LENGTH) {
        buffer[index] += slope_delta * (slope_dd_table[i] + r * (slope_dd_table[i + 1] - slope_dd_table[i]));
        i += MINBLEP_PHASES;
        index++;
    }
}

/* declare the oscillator functions */
#define BLOSC_SINGLE1
#include "xsynth_voice_blosc.h"
#undef BLOSC_SINGLE1
#define BLOSC_MASTER
#include "xsynth_voice_blosc.h"
#undef BLOSC_MASTER
#define BLOSC_SINGLE2
#include "xsynth_voice_blosc.h"
#undef BLOSC_SINGLE2
#define BLOSC_SLAVE
#include "xsynth_voice_blosc.h"
#undef BLOSC_SLAVE

/* vcf_2pole
 *
 * The original Xsynth 12db/oct filter
 */
static inline void
vcf_2pole(xsynth_voice_t *voice, unsigned long sample_count,
          float *in, float *out, float *cutoff, float qres, float *amp)
{
    unsigned long sample;
    float freqcut, highpass,
          delay1 = voice->delay1,
          delay2 = voice->delay2;

    qres = 2.0f - qres * 1.995f;

    for (sample = 0; sample < sample_count; sample++) {

        /* Hal Chamberlin's state variable filter */

        freqcut = cutoff[sample] * 2.0f;

        if (freqcut > VCF_FREQ_MAX) freqcut = VCF_FREQ_MAX;

        delay2 = delay2 + freqcut * delay1;             /* delay2 = lowpass output */
        highpass = in[sample] - delay2 - qres * delay1;
        delay1 = freqcut * highpass + delay1;           /* delay1 = bandpass output */

        /* mix filter output into output buffer */
        out[sample] += delay2 * amp[sample];
    }

    voice->delay1 = delay1;
    voice->delay2 = delay2;
    voice->delay3 = 0.0f;
    voice->delay4 = 0.0f;
    voice->c5 = 0.0f;
}

/* vcf_4pole
 *
 * The original Xsynth 24db/oct filter
 */
static inline void
vcf_4pole(xsynth_voice_t *voice, unsigned long sample_count,
          float *in, float *out, float *cutoff, float qres, float *amp)
{
    unsigned long sample;
    float freqcut, highpass,
          delay1 = voice->delay1,
          delay2 = voice->delay2,
          delay3 = voice->delay3,
          delay4 = voice->delay4;

    qres = 2.0f - qres * 1.995f;

    for (sample = 0; sample < sample_count; sample++) {

        /* Hal Chamberlin's state variable filter */

        freqcut = cutoff[sample] * 2.0f;

        if (freqcut > VCF_FREQ_MAX) freqcut = VCF_FREQ_MAX;

        delay2 = delay2 + freqcut * delay1;             /* delay2/4 = lowpass output */
        highpass = in[sample] - delay2 - qres * delay1;
        delay1 = freqcut * highpass + delay1;           /* delay1/3 = bandpass output */

        delay4 = delay4 + freqcut * delay3;
        highpass = delay2 - delay4 - qres * delay3;
        delay3 = freqcut * highpass + delay3;

        /* mix filter output into output buffer */
        out[sample] += delay4 * amp[sample];
    }

    voice->delay1 = delay1;
    voice->delay2 = delay2;
    voice->delay3 = delay3;
    voice->delay4 = delay4;
    voice->c5 = 0.0f;
}

/* vcf_mvclpf
 *
 * Fons Adriaensen's MVCLPF-3
 */
void
vcf_mvclpf(xsynth_voice_t *voice, unsigned long sample_count,
           float *in, float *out, float *cutoff, float res, float *amp)
{
    unsigned long s;
    float g0, g1, w, x, d,
          delay1 = voice->delay1,
          delay2 = voice->delay2,
          delay3 = voice->delay3,
          delay4 = voice->delay4,
          c5     = voice->c5;

    g0 = 0.5f;  /* g0 = dB_to_amplitude(input_gain_in_db) / 2 */
    g1 = 2.0f;  /* g1 = dB_to_amplitude(output_gain_in_db) * 2 */

    /* res should be 0 to 1 already */

    for (s = 0; s < sample_count; s++) {

        w = cutoff[s];
        if (w < 0.75f) w *= 1.005f - w * (0.624f - w * (0.65f - w * 0.54f));
        else
	{
	    w *= 0.6748f;
            if (w > 0.82f) w = 0.82f;
	}

        x = in[s] * g0 - (4.3f - 0.2f * w) * res * c5 + 1e-10f;
        x /= sqrtf(1.0f + x * x);  /* x = tanh(x) */
        d = w * (x  - delay1) / (1.0f + delay1 * delay1);            
        x = delay1 + 0.77f * d;
        delay1 = x + 0.23f * d;        
        d = w * (x  - delay2) / (1.0f + delay2 * delay2);            
        x = delay2 + 0.77f * d;
        delay2 = x + 0.23f * d;        
        d = w * (x  - delay3) / (1.0f + delay3 * delay3);            
        x = delay3 + 0.77f * d;
        delay3 = x + 0.23f * d;        
        d = w * (x  - delay4);
        x = delay4 + 0.77f * d;
        delay4 = x + 0.23f * d;        
        c5 += 0.85f * (delay4 - c5);

        x = in[s] * g0 - (4.3f - 0.2f * w) * res * c5;
        x /= sqrtf(1.0f + x * x);  /* x = tanh(x) */
        d = w * (x  - delay1) / (1.0f + delay1 * delay1);            
        x = delay1 + 0.77f * d;
        delay1 = x + 0.23f * d;        
        d = w * (x  - delay2) / (1.0f + delay2 * delay2);            
        x = delay2 + 0.77f * d;
        delay2 = x + 0.23f * d;        
        d = w * (x  - delay3) / (1.0f + delay3 * delay3);            
        x = delay3 + 0.77f * d;
        delay3 = x + 0.23f * d;        
        d = w * (x  - delay4);
        x = delay4 + 0.77f * d;
        delay4 = x + 0.23f * d;        
        c5 += 0.85f * (delay4 - c5);

        out[s] += g1 * delay4 * amp[s];
    }

    voice->delay1 = delay1;
    voice->delay2 = delay2;
    voice->delay3 = delay3;
    voice->delay4 = delay4;
    voice->c5     = c5;
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
                  eg2        = voice->eg2;
    unsigned char eg1_phase  = voice->eg1_phase,
                  eg2_phase  = voice->eg2_phase;
    int           osc_index  = voice->osc_index;

    /* temporary variables used in calculating voice */

    float fund_pitch;
    float deltat = 1.0f / (float)synth->sample_rate;
    float freq, freqkey, freqeg1, freqeg2, lfo;

    /* set up synthesis variables from patch */
    float         omega1, omega2;
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
    unsigned char vcf_mode = lrintf(*(synth->vcf_mode));
    float         qres = *(synth->vcf_qres) / 1.995f * voice->pressure;  /* now 0 to 1 */
    float         balance1 = 1.0f - *(synth->osc_balance);
    float         balance2 = *(synth->osc_balance);
    float         vol_out = volume(*(synth->volume));

    fund_pitch = *(synth->glide_time) * voice->target_pitch +
                 (1.0f - *(synth->glide_time)) * voice->prev_pitch;    /* portamento */
    if (do_control_update) {
        voice->prev_pitch = fund_pitch; /* save pitch for next time */
    }

    fund_pitch *= synth->pitch_bend * *(synth->tuning);
    
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

    freq = M_PI_F / (float)synth->sample_rate * fund_pitch * synth->mod_wheel;  /* now (0 to 1) * pi */
    freqkey = freq * *(synth->vcf_cutoff);
    freqeg1 = freq * *(synth->eg1_amount_f);
    freqeg2 = freq * *(synth->eg2_amount_f);

    /* copy some things so oscillator functions can see them */
    voice->osc1.waveform = lrintf(*(synth->osc1_waveform));
    voice->osc1.pw       = *(synth->osc1_pulsewidth);
    voice->osc2.waveform = lrintf(*(synth->osc2_waveform));
    voice->osc2.pw       = *(synth->osc2_pulsewidth);

    /* --- LFO, EG1, and EG2 section */

    for (sample = 0; sample < sample_count; sample++) {

        lfo = oscillator(&lfo_pos, omega3, deltat, lfo_waveform);

        eg1 = eg1_rate_level[eg1_phase] + eg1_one_rate[eg1_phase] * eg1;
        eg2 = eg2_rate_level[eg2_phase] + eg2_one_rate[eg2_phase] * eg2;

        voice->osc2_w_buf[sample] = deltat * omega2 *
                                    (1.0f + eg1 * eg1_amount_o) *
                                    (1.0f + eg2 * eg2_amount_o) *
                                    (1.0f + lfo * lfo_amount_o);

        voice->freqcut_buf[sample] = (freqkey + freqeg1 * eg1 + freqeg2 * eg2) *
                                     (1.0f + lfo * lfo_amount_f);

        voice->vca_buf[sample] = eg1 * vol_out;

        if (!eg1_phase && eg1 > eg1_amp) eg1_phase = 1;  /* flip from attack to decay */
        if (!eg2_phase && eg2 > eg2_amp) eg2_phase = 1;  /* flip from attack to decay */
    }

    /* --- VCO 1 section */

    if (osc_sync)
        blosc_master(sample_count, voice, &voice->osc1,
                     osc_index, balance1, deltat * omega1);
    else
        blosc_single1(sample_count, voice, &voice->osc1,
                      osc_index, balance1, deltat * omega1);

    /* --- VCO 2 section */

    if (osc_sync)
        blosc_slave(sample_count, voice, &voice->osc2,
                     osc_index, balance2, voice->osc2_w_buf);
    else
        blosc_single2(sample_count, voice, &voice->osc2,
                      osc_index, balance2, voice->osc2_w_buf);

    /* --- VCF and VCA section */

    switch (vcf_mode) {
      default:
      case 0:
        vcf_2pole(voice, sample_count, voice->osc_audio + osc_index, out,
                  voice->freqcut_buf, qres, voice->vca_buf);
        break;
      case 1:
        vcf_4pole(voice, sample_count, voice->osc_audio + osc_index, out,
                  voice->freqcut_buf, qres, voice->vca_buf);
        break;
      case 2:
        vcf_mvclpf(voice, sample_count, voice->osc_audio + osc_index, out,
                   voice->freqcut_buf, qres, voice->vca_buf);
        break;
    }

    osc_index += sample_count;

    if (do_control_update) {
        /* do those things should be done only once per control-calculation
         * interval ("nugget"), such as voice check-for-dead, pitch envelope
         * calculations, volume envelope phase transition checks, etc. */

        /* check if we've decayed to nothing, turn off voice if so */
        if (eg1_phase == 2 &&
            voice->vca_buf[sample_count - 1] < 6.26e-6f) {
            /* sound has completed its release phase (>96dB below volume '5' max) */

            XDB_MESSAGE(XDB_NOTE, " xsynth_voice_render check for dead: killing note id %d\n", voice->note_id);
            xsynth_voice_off(voice);
            return; /* we're dead now, so return */
        }

        /* already saved prev_pitch above */

        /* check oscillator audio buffer index, shift buffer if necessary */
        if (osc_index > MINBLEP_BUFFER_LENGTH - (XSYNTH_NUGGET_SIZE + LONGEST_DD_PULSE_LENGTH)) {
            memcpy(voice->osc_audio, voice->osc_audio + osc_index,
                   LONGEST_DD_PULSE_LENGTH * sizeof (float));
            memset(voice->osc_audio + LONGEST_DD_PULSE_LENGTH, 0,
                   (MINBLEP_BUFFER_LENGTH - LONGEST_DD_PULSE_LENGTH) * sizeof (float));
            osc_index = 0;
        }
    }

    /* save things for next time around */

    voice->lfo_pos    = lfo_pos;
    voice->eg1        = eg1;
    voice->eg1_phase  = eg1_phase;
    voice->eg2        = eg2;
    voice->eg2_phase  = eg2_phase;
    voice->osc_index  = osc_index;
}

