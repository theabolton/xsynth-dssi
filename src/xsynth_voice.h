/* Xsynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#ifndef _XSYNTH_VOICE_H
#define _XSYNTH_VOICE_H

#include <string.h>

#include <ladspa.h>
#include "dssi.h"

#include "xsynth_types.h"

/* maximum size of a rendering burst */
#define XSYNTH_NUGGET_SIZE      64

/* minBLEP constants */
/* minBLEP table oversampling factor (must be a power of two): */
#define MINBLEP_PHASES          64
/* MINBLEP_PHASES minus one: */
#define MINBLEP_PHASE_MASK      63
/* length in samples of (truncated) step discontinuity delta: */
#define STEP_DD_PULSE_LENGTH    72
/* length in samples of (truncated) slope discontinuity delta: */
#define SLOPE_DD_PULSE_LENGTH   71
/* the longer of the two above: */
#define LONGEST_DD_PULSE_LENGTH STEP_DD_PULSE_LENGTH
/* MINBLEP_BUFFER_LENGTH must be at least XSYNTH_NUGGET_SIZE plus
 * LONGEST_DD_PULSE_LENGTH, and not less than twice LONGEST_DD_PULSE_LENGTH: */
#define MINBLEP_BUFFER_LENGTH  512
/* delay between start of DD pulse and the discontinuity, in samples: */
#define DD_SAMPLE_DELAY          4

struct _xsynth_patch_t
{
    char          name[31];

    float         osc1_pitch;
    unsigned char osc1_waveform;
    float         osc1_pulsewidth;
    float         osc2_pitch;
    unsigned char osc2_waveform;
    float         osc2_pulsewidth;
    unsigned char osc_sync;
    float         osc_balance;
    float         lfo_frequency;
    unsigned char lfo_waveform;
    float         lfo_amount_o;
    float         lfo_amount_f;
    float         eg1_attack_time;
    float         eg1_decay_time;
    float         eg1_sustain_level;
    float         eg1_release_time;
    float         eg1_vel_sens;
    float         eg1_amount_o;
    float         eg1_amount_f;
    float         eg2_attack_time;
    float         eg2_decay_time;
    float         eg2_sustain_level;
    float         eg2_release_time;
    float         eg2_vel_sens;
    float         eg2_amount_o;
    float         eg2_amount_f;
    float         vcf_cutoff;
    float         vcf_qres;
    unsigned char vcf_mode;
    float         glide_time;
    float         volume;
};

enum xsynth_voice_status
{
    XSYNTH_VOICE_OFF,       /* silent: is not processed by render loop */
    XSYNTH_VOICE_ON,        /* has not received a note off event */
    XSYNTH_VOICE_SUSTAINED, /* has received note off, but sustain controller is on */
    XSYNTH_VOICE_RELEASED   /* had note off, not sustained, in final decay phase of envelopes */
};

struct blosc
{
    int   last_waveform,    /* persistent */
          waveform,         /* comes from LADSPA port each cycle */
          bp_high;          /* persistent */
    float pos,              /* persistent */
          pw;               /* comes from LADSPA port each cycle */
};

/*
 * xsynth_voice_t
 */
struct _xsynth_voice_t
{
    unsigned int  note_id;

    unsigned char status;
    unsigned char key;
    unsigned char velocity;
    unsigned char rvelocity;   /* the note-off velocity */

    /* translated controller values */
    float         pressure;    /* filter resonance multiplier, off = 1.0, full on = 0.0 */

    /* persistent voice state */
    float         prev_pitch,
                  target_pitch,
                  lfo_pos;
    struct blosc  osc1,
                  osc2;
    float         eg1,
                  eg2,
                  delay1,
                  delay2,
                  delay3,
                  delay4,
                  c5;
    unsigned char eg1_phase,
                  eg2_phase;
    int           osc_index;       /* shared index into osc_audio */
    float         osc_audio[MINBLEP_BUFFER_LENGTH];
    float         osc_sync[XSYNTH_NUGGET_SIZE]; /* buffer for sync subsample offsets */
    float         osc2_w_buf[XSYNTH_NUGGET_SIZE];
    float         freqcut_buf[XSYNTH_NUGGET_SIZE];
    float         vca_buf[XSYNTH_NUGGET_SIZE];
};

#define _PLAYING(voice)    ((voice)->status != XSYNTH_VOICE_OFF)
#define _ON(voice)         ((voice)->status == XSYNTH_VOICE_ON)
#define _SUSTAINED(voice)  ((voice)->status == XSYNTH_VOICE_SUSTAINED)
#define _RELEASED(voice)   ((voice)->status == XSYNTH_VOICE_RELEASED)
#define _AVAILABLE(voice)  ((voice)->status == XSYNTH_VOICE_OFF)

extern float xsynth_pitch[128];

typedef struct { float value, delta; } float_value_delta;
extern float_value_delta step_dd_table[];

extern float slope_dd_table[];

/* xsynth_voice.c */
xsynth_voice_t *xsynth_voice_new(xsynth_synth_t *synth);
void            xsynth_voice_note_on(xsynth_synth_t *synth,
                                     xsynth_voice_t *voice,
                                     unsigned char key,
                                     unsigned char velocity);
void            xsynth_voice_remove_held_key(xsynth_synth_t *synth,
                                             unsigned char key);
void            xsynth_voice_note_off(xsynth_synth_t *synth,
                                      xsynth_voice_t *voice,
                                      unsigned char key,
                                      unsigned char rvelocity);
void            xsynth_voice_release_note(xsynth_synth_t *synth,
                                          xsynth_voice_t *voice);
void            xsynth_voice_set_ports(xsynth_synth_t *synth,
                                       xsynth_patch_t *patch);
void            xsynth_voice_update_pressure_mod(xsynth_synth_t *synth,
                                                 xsynth_voice_t *voice);

/* xsynth_voice_render.c */
void xsynth_init_tables(void);
void xsynth_voice_render(xsynth_synth_t *synth, xsynth_voice_t *voice,
                         LADSPA_Data *out, unsigned long sample_count,
                         int do_control_update);

/* inline functions */

/*
 * xsynth_voice_off
 * 
 * Purpose: Turns off a voice immediately, meaning that it is not processed
 * anymore by the render loop.
 */
static inline void
xsynth_voice_off(xsynth_voice_t* voice)
{
    voice->status = XSYNTH_VOICE_OFF;
    /* silence the oscillator buffer for the next use */
    memset(voice->osc_audio, 0, MINBLEP_BUFFER_LENGTH * sizeof(float));
    /* -FIX- decrement active voice count? */
}

/*
 * xsynth_voice_start_voice
 */
static inline void
xsynth_voice_start_voice(xsynth_voice_t *voice)
{
    voice->status = XSYNTH_VOICE_ON;
    /* -FIX- increment active voice count? */
}

#endif /* _XSYNTH_VOICE_H */

