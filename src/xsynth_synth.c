/* Xsynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>

#include "xsynth.h"
#include "xsynth_synth.h"
#include "xsynth_voice.h"
#include "gui_data.h"

/*
 * xsynth_synth_all_voices_off
 *
 * stop processing all notes immediately
 */
void
xsynth_synth_all_voices_off(xsynth_synth_t *synth)
{
    int i;
    xsynth_voice_t *voice;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice)) {
            xsynth_voice_off(voice);
        }
    }
    for (i = 0; i < 8; i++) synth->held_keys[i] = -1;
}

/*
 * xsynth_synth_note_off
 *
 * handle a note off message
 */
void
xsynth_synth_note_off(xsynth_synth_t *synth, unsigned char key, unsigned char rvelocity)
{
    int i;
    xsynth_voice_t *voice;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (synth->monophonic ? (_PLAYING(voice)) :
                                (_ON(voice) && (voice->key == key))) {
            XDB_MESSAGE(XDB_NOTE, " xsynth_synth_note_off: key %d rvel %d voice %d note id %d\n", key, rvelocity, i, voice->note_id);
            xsynth_voice_note_off(synth, voice, key, rvelocity);
        }
    }
}

/*
 * xsynth_synth_all_notes_off
 *
 * put all notes into the released state
 */
void
xsynth_synth_all_notes_off(xsynth_synth_t* synth)
{
    int i;
    xsynth_voice_t *voice;

    /* reset the sustain controller */
    synth->cc[MIDI_CTL_SUSTAIN] = 0;
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_ON(voice) || _SUSTAINED(voice)) {
            xsynth_voice_release_note(synth, voice);
        }
    }
}

/*
 * xsynth_synth_free_voice_by_kill
 *
 * selects a voice for killing. the selection algorithm is a refinement
 * of the algorithm previously in fluid_synth_alloc_voice.
 */
static xsynth_voice_t*
xsynth_synth_free_voice_by_kill(xsynth_synth_t *synth)
{
    int i;
    int best_prio = 10001;
    int this_voice_prio;
    xsynth_voice_t *voice;
    int best_voice_index = -1;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
    
        /* safeguard against an available voice. */
        if (_AVAILABLE(voice))
            return voice;
    
        /* Determine, how 'important' a voice is.
         * Start with an arbitrary number */
        this_voice_prio = 10000;

        if (_RELEASED(voice)) {
            /* This voice is in the release phase. Consider it much less
             * important than a voice which is still held. */
            this_voice_prio -= 2000;
        } else if (_SUSTAINED(voice)) {
            /* The sustain pedal is held down, and this voice is still "on"
             * because of this even though it has received a note off.
             * Consider it less important than voices which have not yet
             * received a note off. This decision is somewhat subjective, but
             * usually the sustain pedal is used to play 'more-voices-than-
             * fingers', and if so, it won't hurt as much to kill one of those
             * voices. */
             this_voice_prio -= 1000;
        };

        /* We are not enthusiastic about releasing voices, which have just been
         * started.  Otherwise hitting a chord may result in killing notes
         * belonging to that very same chord.  So subtract the age of the voice
         * from the priority - an older voice is just a little bit less
         * important than a younger voice. */
        this_voice_prio -= (synth->note_id - voice->note_id);
    
        /* -FIX- not yet implemented:
         * /= take a rough estimate of loudness into account. Louder voices are more important. =/
         * if (voice->volenv_section != FLUID_VOICE_ENVATTACK){
         *     this_voice_prio += voice->volenv_val*1000.;
         * };
         */

        /* check if this voice has less priority than the previous candidate. */
        if (this_voice_prio < best_prio)
            best_voice_index = i,
            best_prio = this_voice_prio;
    }

    if (best_voice_index < 0)
        return NULL;

    voice = synth->voice[best_voice_index];
    XDB_MESSAGE(XDB_NOTE, " xsynth_synth_free_voice_by_kill: no available voices, killing voice %d note id %d\n", best_voice_index, voice->note_id);
    xsynth_voice_off(voice);
    return voice;
}

/*
 * xsynth_synth_alloc_voice
 */
static xsynth_voice_t *
xsynth_synth_alloc_voice(xsynth_synth_t* synth, unsigned char key)
{
    int i;
    xsynth_voice_t* voice;

    /* If there is another voice on the same key, advance it
     * to the release phase to keep our CPU usage low. */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (voice->key == key && (_ON(voice) || _SUSTAINED(voice))) {
            xsynth_voice_release_note(synth, voice);
        }
    }

    /* check if there's an available voice */
    voice = NULL;
    for (i = 0; i < synth->voices; i++) {
        if (_AVAILABLE(synth->voice[i])) {
            voice = synth->voice[i];
            break;
        }
    }

    /* No success yet? Then stop a running voice. */
    if (voice == NULL) {
        voice = xsynth_synth_free_voice_by_kill(synth);
    }

    if (voice == NULL) {
        XDB_MESSAGE(XDB_NOTE, " xsynth_synth_alloc_voice: failed to allocate a voice (key=%d)\n", key);
        return NULL;
    }

    XDB_MESSAGE(XDB_NOTE, " xsynth_synth_alloc_voice: key %d voice %p\n", key, voice);
    return voice;
}

/*
 * xsynth_synth_note_on
 */
void
xsynth_synth_note_on(xsynth_synth_t *synth, unsigned char key, unsigned char velocity)
{
    xsynth_voice_t* voice;

    if (synth->monophonic) {

        voice = synth->voice[0];
        if (_PLAYING(synth->voice[0])) {
            XDB_MESSAGE(XDB_NOTE, " xsynth_synth_note_on: retriggering mono voice on new key %d\n", key);
        }

    } else { /* polyphonic mode */

        voice = xsynth_synth_alloc_voice(synth, key);
        if (voice == NULL)
            return;

    }

    voice->note_id  = synth->note_id++;

    xsynth_voice_note_on(synth, voice, key, velocity);
}

/*
 * xsynth_synth_key_pressure
 */
void
xsynth_synth_key_pressure(xsynth_synth_t *synth, unsigned char key, unsigned char pressure)
{
    int i;
    xsynth_voice_t* voice;

    /* save it for future voices */
    synth->key_pressure[key] = pressure;
    
    /* check if any playing voices need updating */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice) && voice->key == key) {
            xsynth_voice_update_pressure_mod(synth, voice);
        }
    }
}

/*
 * xsynth_synth_damp_voices
 *
 * advance all sustained voices to the release phase (note that this does not
 * clear the sustain controller.)
 */
void
xsynth_synth_damp_voices(xsynth_synth_t* synth)
{
    int i;
    xsynth_voice_t* voice;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_SUSTAINED(voice)) {
            xsynth_voice_release_note(synth, voice);
        }
    }
}

/*
 * xsynth_synth_update_wheel_mod
 */
void
xsynth_synth_update_wheel_mod(xsynth_synth_t* synth)
{
    synth->mod_wheel = 1.0f - (float)synth->cc[MIDI_CTL_MSB_MODWHEEL] / 127.0f;
    /* don't need to check if any playing voices need updating, because it's global */
}

/*
 * xsynth_synth_control_change
 */
void
xsynth_synth_control_change(xsynth_synth_t *synth, unsigned int param, signed int value)
{
    synth->cc[param] = value;

    switch (param) {

      case MIDI_CTL_MSB_MODWHEEL:
        xsynth_synth_update_wheel_mod(synth);
        break;

      case MIDI_CTL_SUSTAIN:
        XDB_MESSAGE(XDB_NOTE, " xsynth_synth_control_change: got sustain control of %d\n", value);
        if (value < 64)
            xsynth_synth_damp_voices(synth);
        break;

      case MIDI_CTL_ALL_SOUNDS_OFF:
        xsynth_synth_all_voices_off(synth);
        break;

      case MIDI_CTL_RESET_CONTROLLERS:
        xsynth_synth_init_controls(synth);
        break;

      case MIDI_CTL_ALL_NOTES_OFF:
        xsynth_synth_all_notes_off(synth);
        break;

      /* what others should we respond to? */

      /* these we ignore (let the host handle):
       *  BANK_SELECT_MSB
       *  BANK_SELECT_LSB
       *  DATA_ENTRY_MSB
       *  NRPN_MSB
       *  NRPN_LSB
       *  RPN_MSB
       *  RPN_LSB
       * -FIX- no! we need RPN (0, 0) Pitch Bend Sensitivity!
       */
    }
}

/*
 * xsynth_synth_channel_pressure
 */
void
xsynth_synth_channel_pressure(xsynth_synth_t *synth, signed int pressure)
{
    int i;
    xsynth_voice_t* voice;

    /* save it for future voices */
    synth->channel_pressure = pressure;

    /* check if any playing voices need updating */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice)) {
            xsynth_voice_update_pressure_mod(synth, voice);
        }
    }
}

/*
 * xsynth_synth_pitch_bend
 */
void
xsynth_synth_pitch_bend(xsynth_synth_t *synth, signed int value)
{
    synth->pitch_wheel = value; /* ALSA pitch bend is already -8192 - 8191 */
    synth->pitch_bend = exp((float)(value * synth->pitch_wheel_sensitivity) /
                            (float)(8192 * 12) * M_LN2);
    /* don't need to check if any playing voices need updating, because it's global */
}

/*
 * xsynth_synth_init_controls
 */
void
xsynth_synth_init_controls(xsynth_synth_t *synth)
{
    int i;
    xsynth_voice_t* voice;

    synth->current_program = -1;

    /* if sustain was on, we need to damp any sustained voices */
    if (XSYNTH_SYNTH_SUSTAINED(synth)) {
        synth->cc[MIDI_CTL_SUSTAIN] = 0;
        xsynth_synth_damp_voices(synth);
    }

    for (i = 0; i < 128; i++) {
        synth->key_pressure[i] = 0;
        synth->cc[i] = 0;
    }
    synth->channel_pressure = 0;
    synth->pitch_wheel_sensitivity = 2;  /* two semi-tones */
    synth->pitch_wheel = 0;
    /* we'd want to set volume (MSB 7 and LSB 39) if it wasn't mapped to a port */
    /* synth->cc[7] = 127;  */
    /* synth->cc[39] = 127; */

    xsynth_synth_update_wheel_mod(synth);
    xsynth_synth_pitch_bend(synth, 0);

    /* check if any playing voices need updating */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
        if (_PLAYING(voice)) {
            xsynth_voice_update_pressure_mod(synth, voice);
        }
    }
}

/*
 * xsynth_synth_select_program
 */
void
xsynth_synth_select_program(xsynth_synth_t *synth, unsigned long bank,
                            unsigned long program)
{
    /* -FIX- no support for banks yet, so we just ignore the bank number */
    if (program >= 128) return;
    synth->current_program = program;
    if (program >= synth->patch_count) {
        xsynth_voice_set_ports(synth, &xsynth_init_voice);
    } else {
        xsynth_voice_set_ports(synth, &synth->patches[program]);
    }
}

/*
 * xsynth_data_friendly_patches
 *
 * give the new user a default set of good patches to get started with
 */
void
xsynth_data_friendly_patches(xsynth_synth_t *synth)
{
    if (!synth->patches) {
        if (!(synth->patches = (xsynth_patch_t *)malloc(128 * sizeof(xsynth_patch_t))))
            return;
    }

    memcpy(synth->patches, friendly_patches, friendly_patch_count * sizeof(xsynth_patch_t));

    synth->patch_count = friendly_patch_count;
}

/*
 * xsynth_synth_set_program_descriptor
 */
int
xsynth_synth_set_program_descriptor(xsynth_synth_t *synth,
                                    DSSI_Program_Descriptor *pd,
                                    unsigned long bank, unsigned long program)
{
    /* -FIX- no support for banks yet, so we just ignore the bank number */
    if (program >= 128 || program >= synth->patch_count || !synth->patches) {
        return 0;
    }
    pd->Bank = bank;
    pd->Program = program;
    pd->Name = synth->patches[program].name;
    return 1;

}

/*
 * xsynth_synth_handle_load
 */
char *
xsynth_synth_handle_load(xsynth_synth_t *synth, const char *value)
{
    FILE *fh;
    int count = 0;

    XDB_MESSAGE(XDB_DATA, " xsynth_synth_handle_load: attempting to load '%s'\n", value);
    /* -FIX- implement bank support */
    if (!synth->patches) {
        if (!(synth->patches = (xsynth_patch_t *)malloc(128 * sizeof(xsynth_patch_t))))
            return dssi_configure_message("load error: could not allocate memory for patch bank");
    }

    if ((fh = fopen(value, "rb")) == NULL) {
        return dssi_configure_message("load error: could not open file '%s'", value);
    }
    while (count < 128 &&
           xsynth_data_read_patch(fh, &synth->patches[count], 0, count))
        count++;
    fclose(fh);

    if (!count) {
        return dssi_configure_message("load error: no patches recognized in patch file '%s'", value);
    }
    if (count > synth->patch_count)
        synth->patch_count = count;

    return NULL;
}

/*
 * xsynth_synth_handle_monophonic
 */
char *
xsynth_synth_handle_monophonic(xsynth_synth_t *synth, const char *value)
{
    int mode = -1;

    if (!strcmp(value, "on")) mode = XSYNTH_MONO_MODE_ON;
    else if (!strcmp(value, "once")) mode = XSYNTH_MONO_MODE_ONCE;
    else if (!strcmp(value, "both")) mode = XSYNTH_MONO_MODE_BOTH;
    else if (!strcmp(value, "off"))  mode = XSYNTH_MONO_MODE_OFF;

    if (mode == -1) {
        return dssi_configure_message("error: monophonic value not recognized");
    }

    if (mode == XSYNTH_MONO_MODE_OFF) {  /* polyphonic mode */

        synth->monophonic = 0;
        synth->voices = synth->polyphony;

    } else {  /* one of the monophonic modes */

        /* -FIX- potential race condition with audio thread; add mutex */
        if (!synth->monophonic) xsynth_synth_all_voices_off(synth);
        synth->monophonic = mode;
        synth->voices = 1;

    }

    return NULL;
}

/*
 * xsynth_synth_handle_polyphony
 */
char *
xsynth_synth_handle_polyphony(xsynth_synth_t *synth, const char *value)
{
    int polyphony = atoi(value);
    int i;
    xsynth_voice_t *voice;

    if (polyphony < 1 || polyphony > XSYNTH_MAX_POLYPHONY) {
        return dssi_configure_message("error: polyphony value out of range");
    }
    /* set the new limit */
    synth->polyphony = polyphony;

    if (!synth->monophonic) {
        synth->voices = polyphony;

        /* turn off any voices above the new limit */
        /* -FIX- potential race condition with audio thread; add mutex */
        for (i = polyphony; i < XSYNTH_MAX_POLYPHONY; i++) {
            voice = synth->voice[i];
            if (_PLAYING(voice)) {
                xsynth_voice_off(voice);
            }
        }
    }

    return NULL;
}

/*
 * xsynth_synth_render_voices
 */
void
xsynth_synth_render_voices(xsynth_synth_t *synth, LADSPA_Data *out, unsigned long sample_count,
                        int do_control_update)
{
    unsigned long i;
    xsynth_voice_t* voice;

    /* clear the buffer */
    for (i = 0; i < sample_count; i++)
        out[i] = 0.0f;

#if defined(XSYNTH_DEBUG) && (XSYNTH_DEBUG & XDB_AUDIO)
out[0] += 0.10f; /* add a 'buzz' to output so there's something audible even when quiescent */
#endif /* defined(XSYNTH_DEBUG) && (XSYNTH_DEBUG & XDB_AUDIO) */

    /* render each active voice */
    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice[i];
    
        if (_PLAYING(voice)) {
            xsynth_voice_render(synth, voice, out, sample_count, do_control_update);
        }
    }
}

