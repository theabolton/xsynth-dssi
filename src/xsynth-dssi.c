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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <ladspa.h>
#include "dssi.h"

#include "xsynth_types.h"
#include "xsynth.h"
#include "xsynth_ports.h"
#include "xsynth_synth.h"
#include "xsynth_voice.h"

static LADSPA_Descriptor *xsynth_LADSPA_descriptor = NULL;
static DSSI_Descriptor   *xsynth_DSSI_descriptor = NULL;

static void xsynth_cleanup(LADSPA_Handle instance);
static void xsynth_run_synth(LADSPA_Handle instance, unsigned long sample_count,
                             snd_seq_event_t *events, unsigned long event_count);

/* ---- LADSPA interface ---- */

/*
 * xsynth_instantiate
 *
 * implements LADSPA (*instantiate)()
 */
static LADSPA_Handle
xsynth_instantiate(const LADSPA_Descriptor *descriptor, unsigned long sample_rate)
{
    xsynth_synth_t *synth = (xsynth_synth_t *)calloc(1, sizeof(xsynth_synth_t));
    int i;

    if (!synth) return NULL;
    for (i = 0; i < XSYNTH_MAX_POLYPHONY; i++) {
        synth->voice[i] = xsynth_voice_new(synth);
        if (!synth->voice[i]) {
            // XDB_MESSAGE(-1, " xsynth_instantiate: out of memory!\n");
            xsynth_cleanup(synth);
            return NULL;
        }
    }

    /* do any per-instance one-time initialization here */
    synth->sample_rate = sample_rate;
    synth->patch_count = 0;
    synth->patches = NULL;

    return (LADSPA_Handle)synth;
}

/*
 * xsynth_connect_port
 *
 * implements LADSPA (*connect_port)()
 */
static void
xsynth_connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data)
{
    xsynth_synth_t *synth = (xsynth_synth_t *)instance;

    switch (port) {
      case XSYNTH_PORT_OUTPUT:             synth->output            = data;  break;
      case XSYNTH_PORT_OSC1_PITCH:         synth->osc1_pitch        = data;  break;
      case XSYNTH_PORT_OSC1_WAVEFORM:      synth->osc1_waveform     = data;  break;
      case XSYNTH_PORT_OSC1_PULSEWIDTH:    synth->osc1_pulsewidth   = data;  break;
      case XSYNTH_PORT_OSC2_PITCH:         synth->osc2_pitch        = data;  break;
      case XSYNTH_PORT_OSC2_WAVEFORM:      synth->osc2_waveform     = data;  break;
      case XSYNTH_PORT_OSC2_PULSEWIDTH:    synth->osc2_pulsewidth   = data;  break;
      case XSYNTH_PORT_OSC_SYNC:           synth->osc_sync          = data;  break;
      case XSYNTH_PORT_OSC_BALANCE:        synth->osc_balance       = data;  break;
      case XSYNTH_PORT_LFO_FREQUENCY:      synth->lfo_frequency     = data;  break;
      case XSYNTH_PORT_LFO_WAVEFORM:       synth->lfo_waveform      = data;  break;
      case XSYNTH_PORT_LFO_AMOUNT_O:       synth->lfo_amount_o      = data;  break;
      case XSYNTH_PORT_LFO_AMOUNT_F:       synth->lfo_amount_f      = data;  break;
      case XSYNTH_PORT_EG1_ATTACK_TIME:    synth->eg1_attack_time   = data;  break;
      case XSYNTH_PORT_EG1_DECAY_TIME:     synth->eg1_decay_time    = data;  break;
      case XSYNTH_PORT_EG1_SUSTAIN_LEVEL:  synth->eg1_sustain_level = data;  break;
      case XSYNTH_PORT_EG1_RELEASE_TIME:   synth->eg1_release_time  = data;  break;
      case XSYNTH_PORT_EG1_AMOUNT_O:       synth->eg1_amount_o      = data;  break;
      case XSYNTH_PORT_EG1_AMOUNT_F:       synth->eg1_amount_f      = data;  break;
      case XSYNTH_PORT_EG2_ATTACK_TIME:    synth->eg2_attack_time   = data;  break;
      case XSYNTH_PORT_EG2_DECAY_TIME:     synth->eg2_decay_time    = data;  break;
      case XSYNTH_PORT_EG2_SUSTAIN_LEVEL:  synth->eg2_sustain_level = data;  break;
      case XSYNTH_PORT_EG2_RELEASE_TIME:   synth->eg2_release_time  = data;  break;
      case XSYNTH_PORT_EG2_AMOUNT_O:       synth->eg2_amount_o      = data;  break;
      case XSYNTH_PORT_EG2_AMOUNT_F:       synth->eg2_amount_f      = data;  break;
      case XSYNTH_PORT_VCF_CUTOFF:         synth->vcf_cutoff        = data;  break;
      case XSYNTH_PORT_VCF_QRES:           synth->vcf_qres          = data;  break;
      case XSYNTH_PORT_VCF_4POLE:          synth->vcf_4pole         = data;  break;
      case XSYNTH_PORT_GLIDE_TIME:         synth->glide_time        = data;  break;
      case XSYNTH_PORT_VOLUME:             synth->volume            = data;  break;

      default:
        break;
    }
}

/*
 * xsynth_activate
 *
 * implements LADSPA (*activate)()
 */
static void
xsynth_activate(LADSPA_Handle instance)
{
    xsynth_synth_t *synth = (xsynth_synth_t *)instance;

    synth->nugget_remains = 0;
    synth->note_id = 0;
    synth->monophonic = 0;
    synth->current_program = -1;
    synth->polyphony = XSYNTH_DEFAULT_POLYPHONY;
    synth->voices = XSYNTH_MAX_POLYPHONY;
    xsynth_synth_all_voices_off(synth);
    synth->voices = XSYNTH_DEFAULT_POLYPHONY;
    xsynth_data_friendly_patches(synth);
    xsynth_synth_init_controls(synth);
    xsynth_synth_select_program(synth, 0, 0); /* initialize the ports */
}

/*
 * xsynth_ladspa_run_wrapper
 *
 * implements LADSPA (*run)() by calling xsynth_run_synth() with no events
 */
static void
xsynth_ladspa_run_wrapper(LADSPA_Handle instance, unsigned long sample_count)
{
    xsynth_run_synth(instance, sample_count, NULL, 0);
}

// optional:
//  void (*run_adding)(LADSPA_Handle Instance,
//                     unsigned long SampleCount);
//  void (*set_run_adding_gain)(LADSPA_Handle Instance,
//                              LADSPA_Data   Gain);

/*
 * xsynth_deactivate
 *
 * implements LADSPA (*deactivate)()
 */
void
xsynth_deactivate(LADSPA_Handle instance)
{
    xsynth_synth_t *synth = (xsynth_synth_t *)instance;

    xsynth_synth_all_voices_off(synth);  /* stop all sounds immediately */
    synth->patch_count = 0;
    if (synth->patches) {
       free(synth->patches);
       synth->patches = NULL;
    }
}

/*
 * xsynth_cleanup
 *
 * implements LADSPA (*cleanup)()
 */
static void
xsynth_cleanup(LADSPA_Handle instance)
{
    xsynth_synth_t *synth = (xsynth_synth_t *)instance;
    int i;

    for (i = 0; i < XSYNTH_MAX_POLYPHONY; i++)
        if (synth->voice[i]) free(synth->voice[i]);
    if (synth->patches) free(synth->patches);
    free(synth);
}

/* ---- DSSI interface ---- */

/*
 * dssi_configure_message
 */
char *
dssi_configure_message(const char *fmt, ...)
{
    va_list args;
    char buffer[256];

    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);
    return strdup(buffer);
}

/*
 * xsynth_configure
 *
 * implements DSSI (*configure)()
 */
char *
xsynth_configure(LADSPA_Handle instance, const char *key, const char *value)
{
    XDB_MESSAGE(XDB_DSSI, " xsynth_configure called with '%s' and '%s'\n", key, value);

    if (!strcmp(key, "load")) {

        return xsynth_synth_handle_load((xsynth_synth_t *)instance, value);

    } else if (!strcmp(key, "monophonic")) {

        return xsynth_synth_handle_monophonic((xsynth_synth_t *)instance, value);

    } else if (!strcmp(key, "polyphony")) {

        return xsynth_synth_handle_polyphony((xsynth_synth_t *)instance, value);

    }
    return strdup("error: unrecognized configure key");
}

/*
 * xsynth_get_program
 *
 * implements DSSI (*get_program)()
 */
const DSSI_Program_Descriptor *
xsynth_get_program(LADSPA_Handle instance, unsigned long index)
{
    xsynth_synth_t *synth = (xsynth_synth_t *)instance;
    static DSSI_Program_Descriptor pd;

    XDB_MESSAGE(XDB_DSSI, " xsynth_get_program called with %lu\n", index);
    /* -FIX- no support for banks yet, so all patches are in bank 0 */
    pd.Bank = 0;
    pd.Program = 0;
    pd.Name = "init voice";
    if (!synth->patch_count)
        return index ? NULL : &pd;
    if (synth->patches && index < synth->patch_count) {
        xsynth_synth_set_program_descriptor(synth, &pd, 0, index);
        return &pd;
    }
    return NULL;
}

/*
 * xsynth_select_program
 *
 * implements DSSI (*select_program)()
 */
void
xsynth_select_program(LADSPA_Handle instance, unsigned long bank,
                      unsigned long program)
{
    XDB_MESSAGE(XDB_DSSI, " xsynth_select_program called with %lu and %lu\n", bank, program);
    xsynth_synth_select_program((xsynth_synth_t *)instance, bank, program);
}

/*
 * xsynth_get_midi_controller
 *
 * implements DSSI (*get_midi_controller_for_port)()
 */
int
xsynth_get_midi_controller(LADSPA_Handle instance, unsigned long port)
{
    XDB_MESSAGE(XDB_DSSI, " xsynth_get_midi_controller called for port %lu\n", port);
    switch (port) {
      case XSYNTH_PORT_VOLUME:
        return DSSI_CC(7);
      default:
        break;
    }

    return DSSI_NONE;
}

/*
 * xsynth_handle_event
 */
static inline void
xsynth_handle_event(xsynth_synth_t *synth, snd_seq_event_t *event)
{
    XDB_MESSAGE(XDB_DSSI, " xsynth_handle_event called with event type %d\n", event->type);
    /* -FIX- there's a potential race condition here with host thread calling
     * configure(); add mutex, if trylock fails, do all_voices_off, flush event,
     * and continue */
    switch (event->type) {
      case SND_SEQ_EVENT_NOTEOFF:
        xsynth_synth_note_off(synth, event->data.note.note, event->data.note.velocity);
        break;
      case SND_SEQ_EVENT_NOTEON:
        if (event->data.note.velocity > 0)
           xsynth_synth_note_on(synth, event->data.note.note, event->data.note.velocity);
        else
           xsynth_synth_note_off(synth, event->data.note.note, 64); /* shouldn't happen, but... */
        break;
      case SND_SEQ_EVENT_KEYPRESS:
        xsynth_synth_key_pressure(synth, event->data.note.note, event->data.note.velocity);
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        xsynth_synth_control_change(synth, event->data.control.param, event->data.control.value);
        break;
      case SND_SEQ_EVENT_CHANPRESS:
        xsynth_synth_channel_pressure(synth, event->data.control.value);
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        xsynth_synth_pitch_bend(synth, event->data.control.value);
        break;
      /* SND_SEQ_EVENT_PGMCHANGE - shouldn't happen */
      /* SND_SEQ_EVENT_SYSEX - shouldn't happen */
      /* SND_SEQ_EVENT_CONTROL14? */
      /* SND_SEQ_EVENT_NONREGPARAM? */
      /* SND_SEQ_EVENT_REGPARAM? */
      default:
        break;
    }
}

/*
 * xsynth_run_synth
 *
 * implements DSSI (*run_synth)()
 */
static void
xsynth_run_synth(LADSPA_Handle instance, unsigned long sample_count,
                 snd_seq_event_t *events, unsigned long event_count)
{
    xsynth_synth_t *synth = (xsynth_synth_t *)instance;
    unsigned long samples_done = 0;
    unsigned long event_index = 0;
    unsigned long burst_size;

    while (samples_done < sample_count) {
        if (!synth->nugget_remains)
            synth->nugget_remains = XSYNTH_NUGGET_SIZE;

        /* process any ready events */
	while (event_index < event_count
	       && samples_done == events[event_index].time.tick) {
            xsynth_handle_event(synth, &events[event_index]);
            event_index++;
        }

        /* calculate the sample count (burst_size) for the next
         * xsynth_voice_render() call to be the smallest of:
         * - control calculation quantization size (XSYNTH_NUGGET_SIZE, in
         *     samples)
         * - the number of samples remaining in an already-begun nugget
         *     (synth->nugget_remains)
         * - the number of samples until the next event is ready
         * - the number of samples left in this run
         */
        burst_size = XSYNTH_NUGGET_SIZE;
        if (synth->nugget_remains < burst_size) {
            /* we're still in the middle of a nugget, so reduce the burst size
             * to end when the nugget ends */
            burst_size = synth->nugget_remains;
        }
        if (event_index < event_count
            && events[event_index].time.tick - samples_done < burst_size) {
            /* reduce burst size to end when next event is ready */
            burst_size = events[event_index].time.tick - samples_done;
        }
        if (sample_count - samples_done < burst_size) {
            /* reduce burst size to end at end of this run */
            burst_size = sample_count - samples_done;
        }

        /* render the burst */
        xsynth_synth_render_voices(synth, synth->output + samples_done, burst_size,
                                (burst_size == synth->nugget_remains));
        samples_done += burst_size;
        synth->nugget_remains -= burst_size;
    }
#if defined(XSYNTH_DEBUG) && (XSYNTH_DEBUG & XDB_AUDIO)
*synth->output += 0.10f; /* add a 'buzz' to output so there's something audible even when quiescent */
#endif /* defined(XSYNTH_DEBUG) && (XSYNTH_DEBUG & XDB_AUDIO) */
}

// optional:
//    void (*run_synth_adding)(LADSPA_Handle    Instance,
//                             unsigned long    SampleCount,
//                             snd_seq_event_t *Events,
//                             unsigned long    EventCount);

/* ---- export ---- */

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
        return xsynth_LADSPA_descriptor;
    default:
        return NULL;
    }
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
        return xsynth_DSSI_descriptor;
    default:
        return NULL;
    }
}

void _init()
{
    int i;
    char **port_names;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;

    XSYNTH_DEBUG_INIT("xsynth-dssi.so");

    xsynth_init_waveforms();
    xsynth_pitch_init();
    xsynth_volume_init();

    xsynth_LADSPA_descriptor =
        (LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));
    if (xsynth_LADSPA_descriptor) {
        xsynth_LADSPA_descriptor->UniqueID = 2181;
        xsynth_LADSPA_descriptor->Label = "Xsynth";
        xsynth_LADSPA_descriptor->Properties = 0;
        xsynth_LADSPA_descriptor->Name = "Xsynth DSSI plugin";
        xsynth_LADSPA_descriptor->Maker = "Sean Bolton <musound AT jps DOT net>";
        xsynth_LADSPA_descriptor->Copyright = "GNU General Public License version 2 or later";
        xsynth_LADSPA_descriptor->PortCount = XSYNTH_PORTS_COUNT;

        port_descriptors = (LADSPA_PortDescriptor *)
                                calloc(xsynth_LADSPA_descriptor->PortCount, sizeof
                                                (LADSPA_PortDescriptor));
        xsynth_LADSPA_descriptor->PortDescriptors =
            (const LADSPA_PortDescriptor *) port_descriptors;

        port_range_hints = (LADSPA_PortRangeHint *)
                                calloc(xsynth_LADSPA_descriptor->PortCount, sizeof
                                                (LADSPA_PortRangeHint));
        xsynth_LADSPA_descriptor->PortRangeHints =
            (const LADSPA_PortRangeHint *) port_range_hints;

        port_names = (char **) calloc(xsynth_LADSPA_descriptor->PortCount, sizeof(char *));
        xsynth_LADSPA_descriptor->PortNames = (const char **) port_names;

        for (i = 0; i < XSYNTH_PORTS_COUNT; i++) {
            port_descriptors[i] = xsynth_port_description[i].port_descriptor;
            port_names[i]       = xsynth_port_description[i].name;
            port_range_hints[i].HintDescriptor = xsynth_port_description[i].hint_descriptor;
            port_range_hints[i].LowerBound     = xsynth_port_description[i].lower_bound;
            port_range_hints[i].UpperBound     = xsynth_port_description[i].upper_bound;
        }

        xsynth_LADSPA_descriptor->instantiate = xsynth_instantiate;
        xsynth_LADSPA_descriptor->connect_port = xsynth_connect_port;
        xsynth_LADSPA_descriptor->activate = xsynth_activate;
        xsynth_LADSPA_descriptor->run = xsynth_ladspa_run_wrapper;
        xsynth_LADSPA_descriptor->run_adding = NULL;
        xsynth_LADSPA_descriptor->set_run_adding_gain = NULL;
        xsynth_LADSPA_descriptor->deactivate = xsynth_deactivate;
        xsynth_LADSPA_descriptor->cleanup = xsynth_cleanup;
    }

    xsynth_DSSI_descriptor = (DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));
    if (xsynth_DSSI_descriptor) {
        xsynth_DSSI_descriptor->DSSI_API_Version = 1;
        xsynth_DSSI_descriptor->LADSPA_Plugin = xsynth_LADSPA_descriptor;
        xsynth_DSSI_descriptor->configure = xsynth_configure;
        xsynth_DSSI_descriptor->get_program = xsynth_get_program;
        xsynth_DSSI_descriptor->select_program = xsynth_select_program;
        xsynth_DSSI_descriptor->get_midi_controller_for_port = xsynth_get_midi_controller;
        xsynth_DSSI_descriptor->run_synth = xsynth_run_synth;
        xsynth_DSSI_descriptor->run_synth_adding = NULL;
        xsynth_DSSI_descriptor->run_multiple_synths = NULL;
        xsynth_DSSI_descriptor->run_multiple_synths_adding = NULL;
    }
}

void _fini()
{
    if (xsynth_LADSPA_descriptor) {
        free((LADSPA_PortDescriptor *) xsynth_LADSPA_descriptor->PortDescriptors);
        free((char **) xsynth_LADSPA_descriptor->PortNames);
        free((LADSPA_PortRangeHint *) xsynth_LADSPA_descriptor->PortRangeHints);
        free(xsynth_LADSPA_descriptor);
    }
    if (xsynth_DSSI_descriptor) {
        free(xsynth_DSSI_descriptor);
    }
}

