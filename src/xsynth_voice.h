/* Xsynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _XSYNTH_VOICE_H
#define _XSYNTH_VOICE_H

#include <ladspa.h>
#include "dssi.h"

#include "xsynth_types.h"

#define WAVE_POINTS 100

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
    unsigned char vcf_4pole;
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
                  lfo_pos,
                  osc1_pos,
                  osc2_pos,
                  eg1,
                  eg2,
                  delay1,
                  delay2,
                  delay3,
                  delay4;
    unsigned char eg1_phase,
                  eg2_phase;
};

#define _PLAYING(voice)    ((voice)->status != XSYNTH_VOICE_OFF)
#define _ON(voice)         ((voice)->status == XSYNTH_VOICE_ON)
#define _SUSTAINED(voice)  ((voice)->status == XSYNTH_VOICE_SUSTAINED)
#define _RELEASED(voice)   ((voice)->status == XSYNTH_VOICE_RELEASED)
#define _AVAILABLE(voice)  ((voice)->status == XSYNTH_VOICE_OFF)

extern float          xsynth_pitch[128];

/* xsynth_voice.c */
xsynth_voice_t *xsynth_voice_new(xsynth_synth_t *synth);
void            xsynth_voice_note_on(xsynth_synth_t *synth,
                                     xsynth_voice_t *voice,
                                     unsigned char key,
                                     unsigned char velocity);
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

