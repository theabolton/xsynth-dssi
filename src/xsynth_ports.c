/* Xsynth DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * Xsynth, copyright (C) 1999 S. J. Brookes.
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

#include <ladspa.h>

#include "xsynth_ports.h"

struct xsynth_port_descriptor xsynth_port_description[XSYNTH_PORTS_COUNT] = {

#define PD_OUT     (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO)
#define PD_IN      (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL)
#define HD_MIN     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM)
#define HD_LOW     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW)
#define HD_MID     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE)
#define HD_HI      (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH)
#define HD_MAX     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM)
#define HD_LOG     (LADSPA_HINT_LOGARITHMIC)
#define HD_DETENT  (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MINIMUM)
#define HD_SWITCH  (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_TOGGLED )
#define XPT_LIN    XSYNTH_PORT_TYPE_LINEAR
#define XPT_LOG    XSYNTH_PORT_TYPE_LOGARITHMIC
#define XPT_DETE   XSYNTH_PORT_TYPE_DETENT
#define XPT_ONOFF  XSYNTH_PORT_TYPE_ONOFF
    { PD_OUT, "Output",              0,               0.0f,     0.0f,   0,         0.,0.,0. },
    { PD_IN,  "OSC1 Pitch",          HD_MAX | HD_LOG, 1.0f,     4.0f,   XPT_LOG,   1.,2.,2. },
    { PD_IN,  "OSC1 Waveform",       HD_DETENT,       0.0f,     5.0f,   XPT_DETE,  0.,0.,0. },
    { PD_IN,  "OSC1 Pulse Width",    HD_MID,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "OSC2 Pitch",          HD_MAX | HD_LOG, 1.0f,     4.0f,   XPT_LOG,   1.,2.,2. },
    { PD_IN,  "OSC2 Waveform",       HD_DETENT,       0.0f,     5.0f,   XPT_DETE,  0.,0.,0. },
    { PD_IN,  "OSC2 Pulse Width",    HD_MID,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "Oscillator Sync",     HD_SWITCH,       0.0f,     1.0f,   XPT_ONOFF, 0.,0.,0. },
    { PD_IN,  "Oscillator Balance",  HD_MIN,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "LFO Frequency",       HD_MID | HD_LOG, 0.1f,     10.0f,  XPT_LOG,   0.1,10.,2. },
    { PD_IN,  "LFO Waveform",        HD_DETENT,       0.0f,     5.0f,   XPT_DETE,  0.,0.,0. },
    { PD_IN,  "LFO Osc Pitch Mod",   HD_MIN,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "LFO VCF Cutoff Mod",  HD_MIN,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "EG1 Attack Rate",     HD_HI | HD_LOG,  0.00001f, 0.1f,   XPT_LOG,   0.1,10.,-4. },
    { PD_IN,  "EG1 Decay Rate",      HD_HI | HD_LOG,  0.00001f, 0.1f,   XPT_LOG,   0.1,10.,-4. },
    { PD_IN,  "EG1 Sustain Level",   HD_MID,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "EG1 Release Rate",    HD_HI | HD_LOG,  0.00001f, 0.1f,   XPT_LOG,   0.1,10.,-4. },
    { PD_IN,  "EG1 Osc Pitch Mod",   HD_MIN,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "EG1 VCF Cutoff Mod",  HD_MIN,          0.0f,     50.0f,  XPT_LIN,   0.,50.,0. },
    { PD_IN,  "EG2 Attack Rate",     HD_HI | HD_LOG,  0.00001f, 0.1f,   XPT_LOG,   0.1,10.,-4. },
    { PD_IN,  "EG2 Decay Rate",      HD_HI | HD_LOG,  0.00001f, 0.1f,   XPT_LOG,   0.1,10.,-4. },
    { PD_IN,  "EG2 Sustain Level",   HD_MID,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "EG2 Release Rate",    HD_HI | HD_LOG,  0.00001f, 0.1f,   XPT_LOG,   0.1,10.,-4. },
    { PD_IN,  "EG2 Osc Pitch Mod",   HD_MIN,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. },
    { PD_IN,  "EG2 VCF Cutoff Mod",  HD_MIN,          0.0f,     50.0f,  XPT_LIN,   0.,50.,0. },
    { PD_IN,  "VCF Cutoff",          HD_MAX,          0.0f,     50.0f,  XPT_LIN,   0.,50.,0. },
    { PD_IN,  "VCF Resonance",       HD_MIN,          0.0f,     1.995f, XPT_LIN,   0.,1.995,0. },
    { PD_IN,  "VCF dB/oct 12 or 24", HD_SWITCH,       0.0f,     1.0f,   XPT_ONOFF, 0.,0.,0. },
    { PD_IN,  "Glide Rate",          HD_MIN | HD_LOG, 0.002f,   1.0f,   XPT_LOG,   1.,0.002,1. },  // -FIX- this needs to be adjusted for different cx rates!
    { PD_IN,  "Volume",              HD_LOW,          0.0f,     1.0f,   XPT_LIN,   0.,1.,0. }
#undef PD_OUT
#undef PD_IN
#undef HD_MIN
#undef HD_LOW
#undef HD_MAX
#undef HD_LOG
#undef HD_DETENT
#undef HD_SWITCH
#undef XPT_LIN
#undef XPT_LOG
#undef XPT_DETE
#undef XPT_ONOFF
};

