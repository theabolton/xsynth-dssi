/* Xsynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton.
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

#ifndef _GUI_IMAGES_H
#define _GUI_IMAGES_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

/* gui_images.c */
GtkWidget *create_about_pixmap(GtkWidget *window);
GtkWidget *create_logo_pixmap(GtkWidget *window);

void       create_waveform_gdk_pixmaps(GtkWidget *widget);
void       free_waveform_gdk_pixmaps(void);
GtkWidget* create_waveform_pixmap(GtkWidget *widget);
void       set_waveform_pixmap(GtkWidget *widget, int waveform);

#endif /* _GUI_IMAGES_H */
