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
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _XSYNTH_PORTS_H
#define _XSYNTH_PORTS_H

#include <ladspa.h>

#define XSYNTH_PORT_OUTPUT              0
#define XSYNTH_PORT_OSC1_PITCH          1
#define XSYNTH_PORT_OSC1_WAVEFORM       2
#define XSYNTH_PORT_OSC1_PULSEWIDTH     3
#define XSYNTH_PORT_OSC2_PITCH          4
#define XSYNTH_PORT_OSC2_WAVEFORM       5
#define XSYNTH_PORT_OSC2_PULSEWIDTH     6
#define XSYNTH_PORT_OSC_SYNC            7
#define XSYNTH_PORT_OSC_BALANCE         8
#define XSYNTH_PORT_LFO_FREQUENCY       9
#define XSYNTH_PORT_LFO_WAVEFORM       10
#define XSYNTH_PORT_LFO_AMOUNT_O       11
#define XSYNTH_PORT_LFO_AMOUNT_F       12
#define XSYNTH_PORT_EG1_ATTACK_TIME    13
#define XSYNTH_PORT_EG1_DECAY_TIME     14
#define XSYNTH_PORT_EG1_SUSTAIN_LEVEL  15
#define XSYNTH_PORT_EG1_RELEASE_TIME   16
#define XSYNTH_PORT_EG1_AMOUNT_O       17
#define XSYNTH_PORT_EG1_AMOUNT_F       18
#define XSYNTH_PORT_EG2_ATTACK_TIME    19
#define XSYNTH_PORT_EG2_DECAY_TIME     20
#define XSYNTH_PORT_EG2_SUSTAIN_LEVEL  21
#define XSYNTH_PORT_EG2_RELEASE_TIME   22
#define XSYNTH_PORT_EG2_AMOUNT_O       23
#define XSYNTH_PORT_EG2_AMOUNT_F       24
#define XSYNTH_PORT_VCF_CUTOFF         25
#define XSYNTH_PORT_VCF_QRES           26
#define XSYNTH_PORT_VCF_MODE           27
#define XSYNTH_PORT_GLIDE_TIME         28
#define XSYNTH_PORT_VOLUME             29
/* added in v0.1.1: */
#define XSYNTH_PORT_TUNING             30
#define XSYNTH_PORT_EG1_VEL_SENS       31
#define XSYNTH_PORT_EG2_VEL_SENS       32

#define XSYNTH_PORTS_COUNT  33

#define XSYNTH_PORT_TYPE_LINEAR       0
#define XSYNTH_PORT_TYPE_LOGARITHMIC  1
#define XSYNTH_PORT_TYPE_DETENT       2
#define XSYNTH_PORT_TYPE_ONOFF        3
#define XSYNTH_PORT_TYPE_VCF_MODE     4

struct xsynth_port_descriptor {

    LADSPA_PortDescriptor          port_descriptor;
    char *                         name;
    LADSPA_PortRangeHintDescriptor hint_descriptor;
    LADSPA_Data                    lower_bound;
    LADSPA_Data                    upper_bound;
    int                            type;
    float                          a, b, c;  /* scaling parameters for continuous controls */

};

extern struct xsynth_port_descriptor xsynth_port_description[];

#endif /* _XSYNTH_PORTS_H */

