/* Xsynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
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

#ifndef _GUI_DATA_H
#define _GUI_DATA_H

#include <stdio.h>

#include "xsynth_types.h"
#include "xsynth_voice.h"

#define XSYNTH_OLD_PATCH_SIZE_PACKED     209  /* Steve Brookes' original patch save file format */
#define XSYNTH_OLD_PATCH_SIZE_WITH_NAME  224  /* the above followed by 15 characters of name */

/* gui_data.c */
void gui_data_import_patch(xsynth_patch_t *xsynth_patch,
                           unsigned char *old_patch, int unpack_name);
int  gui_data_write_patch(FILE *file, xsynth_patch_t *patch);
int  gui_data_save(char *filename, int start, int end, char **message);
void gui_data_mark_dirty_patch_sections(int start_patch, int end_patch);
int  gui_data_load(const char *filename, int position, char **message);
void gui_data_friendly_patches(void);
void gui_data_send_dirty_patch_sections(void);
int  gui_data_patch_compare(xsynth_patch_t *patch1, xsynth_patch_t *patch2);

/* gui_friendly_patches.c */
extern int            friendly_patch_count;
extern xsynth_patch_t friendly_patches[];

/* xsynth_data.c */
extern xsynth_patch_t xsynth_init_voice;

int   xsynth_data_read_patch(FILE *file, xsynth_patch_t *patch);
int   xsynth_data_decode_patches(const char *encoded, xsynth_patch_t *patches);

#endif /* _GUI_DATA_H */

