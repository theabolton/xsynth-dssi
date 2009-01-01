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

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>

#include "xsynth_types.h"
#include "xsynth.h"
#include "xsynth_synth.h"
#include "xsynth_voice.h"

/*
 * xsynth_voice_new
 */
xsynth_voice_t *
xsynth_voice_new(xsynth_synth_t *synth)
{
    xsynth_voice_t *voice;

    voice = (xsynth_voice_t *)calloc(sizeof(xsynth_voice_t), 1);
    if (voice) {
        voice->status = XSYNTH_VOICE_OFF;
    }
    return voice;
}

/*
 * xsynth_voice_note_on
 */
void
xsynth_voice_note_on(xsynth_synth_t *synth, xsynth_voice_t *voice,
                     unsigned char key, unsigned char velocity)
{
    int i;

    voice->key      = key;
    voice->velocity = velocity;

    if (!synth->monophonic || !(_ON(voice) || _SUSTAINED(voice))) {

        /* brand-new voice, or monophonic voice in release phase; set
         * everything up */
        XDB_MESSAGE(XDB_NOTE, " xsynth_voice_note_on in polyphonic/new section: key %d, mono %d, old status %d\n", key, synth->monophonic, voice->status);

        voice->target_pitch = xsynth_pitch[key];
        switch(synth->glide) {
          case XSYNTH_GLIDE_MODE_LEGATO:
            if (synth->held_keys[0] >= 0) {
                voice->prev_pitch = xsynth_pitch[synth->held_keys[0]];
            } else {
                voice->prev_pitch = voice->target_pitch;
            }
            break;

          case XSYNTH_GLIDE_MODE_INITIAL:
            if (synth->held_keys[0] >= 0) {
                voice->prev_pitch = voice->target_pitch;
            } else {
                voice->prev_pitch = synth->last_noteon_pitch;
            }
            break;

          case XSYNTH_GLIDE_MODE_ALWAYS:
            if (synth->held_keys[0] >= 0) {
                voice->prev_pitch = xsynth_pitch[synth->held_keys[0]];
            } else {
                voice->prev_pitch = synth->last_noteon_pitch;
            }
            break;

          case XSYNTH_GLIDE_MODE_LEFTOVER:
            /* leave voice->prev_pitch at whatever it was */
            break;

          default:
          case XSYNTH_GLIDE_MODE_OFF:
            voice->prev_pitch = voice->target_pitch;
            break;
        }
        if (!_PLAYING(voice)) {
            voice->lfo_pos = 0.0f;
            voice->eg1 = 0.0f;
            voice->eg2 = 0.0f;
            voice->delay1 = 0.0f;
            voice->delay2 = 0.0f;
            voice->delay3 = 0.0f;
            voice->delay4 = 0.0f;
            voice->c5     = 0.0f;
            voice->osc_index = 0;
            voice->osc1.last_waveform = -1;
            voice->osc1.pos = 0.0f;
            voice->osc2.last_waveform = -1;
            voice->osc2.pos = 0.0f;
        }
        voice->eg1_phase = 0;
        voice->eg2_phase = 0;
        xsynth_voice_update_pressure_mod(synth, voice);

    } else {

        /* synth is monophonic, and we're modifying a playing voice */
        XDB_MESSAGE(XDB_NOTE, " xsynth_voice_note_on in monophonic section: old key %d => new key %d\n", synth->held_keys[0], key);

        /* set new pitch */
        voice->target_pitch = xsynth_pitch[key];
        if (synth->glide == XSYNTH_GLIDE_MODE_INITIAL ||
            synth->glide == XSYNTH_GLIDE_MODE_OFF)
            voice->prev_pitch = voice->target_pitch;

        /* if in 'on' or 'both' modes, and key has changed, then re-trigger EGs */
        if ((synth->monophonic == XSYNTH_MONO_MODE_ON ||
             synth->monophonic == XSYNTH_MONO_MODE_BOTH) &&
            (synth->held_keys[0] < 0 || synth->held_keys[0] != key)) {
            voice->eg1_phase = 0;
            voice->eg2_phase = 0;
        }

        /* all other variables stay what they are */

    }
    synth->last_noteon_pitch = voice->target_pitch;

    /* add new key to the list of held keys */

    /* check if new key is already in the list; if so, move it to the
     * top of the list, otherwise shift the other keys down and add it
     * to the top of the list. */
    for (i = 0; i < 7; i++) {
        if (synth->held_keys[i] == key)
            break;
    }
    for (; i > 0; i--) {
        synth->held_keys[i] = synth->held_keys[i - 1];
    }
    synth->held_keys[0] = key;

    if (!_PLAYING(voice)) {

        xsynth_voice_start_voice(voice);

    } else if (!_ON(voice)) {  /* must be XSYNTH_VOICE_SUSTAINED or XSYNTH_VOICE_RELEASED */

        voice->status = XSYNTH_VOICE_ON;

    }
}

/*
 * xsynth_voice_set_release_phase
 */
static inline void
xsynth_voice_set_release_phase(xsynth_voice_t *voice)
{
    voice->eg1_phase = 2;
    voice->eg2_phase = 2;
}

/*
 * xsynth_voice_remove_held_key
 */
inline void
xsynth_voice_remove_held_key(xsynth_synth_t *synth, unsigned char key)
{
    int i;

    /* check if this key is in list of held keys; if so, remove it and
     * shift the other keys up */
    for (i = 7; i >= 0; i--) {
        if (synth->held_keys[i] == key)
            break;
    }
    if (i >= 0) {
        for (; i < 7; i++) {
            synth->held_keys[i] = synth->held_keys[i + 1];
        }
        synth->held_keys[7] = -1;
    }
}

/*
 * xsynth_voice_note_off
 */
void
xsynth_voice_note_off(xsynth_synth_t *synth, xsynth_voice_t *voice,
                      unsigned char key, unsigned char rvelocity)
{
    unsigned char previous_top_key;

    XDB_MESSAGE(XDB_NOTE, " xsynth_set_note_off: called for voice %p, key %d\n", voice, key);

    /* save release velocity */
    voice->rvelocity = rvelocity;

    previous_top_key = synth->held_keys[0];

    /* remove this key from list of held keys */
    xsynth_voice_remove_held_key(synth, key);

    if (synth->monophonic) {  /* monophonic mode */

        if (synth->held_keys[0] >= 0) {

            /* still some keys held */

            if (synth->held_keys[0] != previous_top_key) {

                /* most-recently-played key has changed */
                voice->key = synth->held_keys[0];
                XDB_MESSAGE(XDB_NOTE, " note-off in monophonic section: changing pitch to %d\n", voice->key);
                voice->target_pitch = xsynth_pitch[voice->key];
                if (synth->glide == XSYNTH_GLIDE_MODE_INITIAL ||
                    synth->glide == XSYNTH_GLIDE_MODE_OFF)
                    voice->prev_pitch = voice->target_pitch;

                /* if mono mode is 'both', re-trigger EGs */
                if (synth->monophonic == XSYNTH_MONO_MODE_BOTH && !_RELEASED(voice)) {
                    voice->eg1_phase = 0;
                    voice->eg2_phase = 0;
                }

            }

        } else {  /* no keys still held */

            if (XSYNTH_SYNTH_SUSTAINED(synth)) {

                /* no more keys in list, but we're sustained */
                XDB_MESSAGE(XDB_NOTE, " note-off in monophonic section: sustained with no held keys\n");
                if (!_RELEASED(voice))
                    voice->status = XSYNTH_VOICE_SUSTAINED;

            } else {  /* not sustained */

                /* no more keys in list, so turn off note */
                XDB_MESSAGE(XDB_NOTE, " note-off in monophonic section: turning off voice %p\n", voice);
                xsynth_voice_set_release_phase(voice);
                voice->status = XSYNTH_VOICE_RELEASED;

            }
        }

    } else {  /* polyphonic mode */

        if (XSYNTH_SYNTH_SUSTAINED(synth)) {

            if (!_RELEASED(voice))
                voice->status = XSYNTH_VOICE_SUSTAINED;

        } else {  /* not sustained */

            xsynth_voice_set_release_phase(voice);
            voice->status = XSYNTH_VOICE_RELEASED;

        }
    }
}

/*
 * xsynth_voice_release_note
 */
void
xsynth_voice_release_note(xsynth_synth_t *synth, xsynth_voice_t *voice)
{
    XDB_MESSAGE(XDB_NOTE, " xsynth_voice_release_note: turning off voice %p\n", voice);
    if (_ON(voice)) {
        /* dummy up a release velocity */
        voice->rvelocity = 64;
    }
    xsynth_voice_set_release_phase(voice);
    voice->status = XSYNTH_VOICE_RELEASED;
}

/*
 * xsynth_voice_set_ports
 */
void
xsynth_voice_set_ports(xsynth_synth_t *synth, xsynth_patch_t *patch)
{
    *(synth->osc1_pitch)        = patch->osc1_pitch;
    *(synth->osc1_waveform)     = (float)patch->osc1_waveform;
    *(synth->osc1_pulsewidth)   = patch->osc1_pulsewidth;
    *(synth->osc2_pitch)        = patch->osc2_pitch;
    *(synth->osc2_waveform)     = (float)patch->osc2_waveform;
    *(synth->osc2_pulsewidth)   = patch->osc2_pulsewidth;
    *(synth->osc_sync)          = (float)patch->osc_sync;
    *(synth->osc_balance)       = patch->osc_balance;
    *(synth->lfo_frequency)     = patch->lfo_frequency;
    *(synth->lfo_waveform)      = (float)patch->lfo_waveform;
    *(synth->lfo_amount_o)      = patch->lfo_amount_o;
    *(synth->lfo_amount_f)      = patch->lfo_amount_f;
    *(synth->eg1_attack_time)   = patch->eg1_attack_time;
    *(synth->eg1_decay_time)    = patch->eg1_decay_time;
    *(synth->eg1_sustain_level) = patch->eg1_sustain_level;
    *(synth->eg1_release_time)  = patch->eg1_release_time;
    *(synth->eg1_vel_sens)      = patch->eg1_vel_sens;
    *(synth->eg1_amount_o)      = patch->eg1_amount_o;
    *(synth->eg1_amount_f)      = patch->eg1_amount_f;
    *(synth->eg2_attack_time)   = patch->eg2_attack_time;
    *(synth->eg2_decay_time)    = patch->eg2_decay_time;
    *(synth->eg2_sustain_level) = patch->eg2_sustain_level;
    *(synth->eg2_release_time)  = patch->eg2_release_time;
    *(synth->eg2_vel_sens)      = patch->eg2_vel_sens;
    *(synth->eg2_amount_o)      = patch->eg2_amount_o;
    *(synth->eg2_amount_f)      = patch->eg2_amount_f;
    *(synth->vcf_cutoff)        = patch->vcf_cutoff;
    *(synth->vcf_qres)          = patch->vcf_qres;
    *(synth->vcf_mode)          = (float)patch->vcf_mode;
    *(synth->glide_time)        = patch->glide_time;
    *(synth->volume)            = patch->volume;
}

/*
 * xsynth_voice_update_pressure_mod
 */
void
xsynth_voice_update_pressure_mod(xsynth_synth_t *synth, xsynth_voice_t *voice)
{
    unsigned char kp = synth->key_pressure[voice->key];
    unsigned char cp = synth->channel_pressure;
    float p;

    /* add the channel and key pressures together in a way that 'feels' good */
    if (kp > cp) {
        p = (float)kp / 127.0f;
        p += (1.0f - p) * ((float)cp / 127.0f);
    } else {
        p = (float)cp / 127.0f;
        p += (1.0f - p) * ((float)kp / 127.0f);
    }
    /* set the pressure modifier so it ranges between 1.0 (no pressure, no
     * resonance boost) and 0.25 (full pressure, resonance boost 75% of the way
     * to filter oscillation) */
    voice->pressure = 1.0f - (p * 0.75f);
}

