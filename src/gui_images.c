/* Xsynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gui_images.h"

#include "bitmap_about.xbm"

#include "bitmap_logo.xbm"

#include "bitmap_waveform0.xpm"
#include "bitmap_waveform1.xpm"
#include "bitmap_waveform2.xpm"
#include "bitmap_waveform3.xpm"
#include "bitmap_waveform4.xpm"
#include "bitmap_waveform5.xpm"
#include "bitmap_waveform6.xpm"

static char **waveform_xpms[7] = {
    waveform0_xpm,
    waveform1_xpm,
    waveform2_xpm,
    waveform3_xpm,
    waveform4_xpm,
    waveform5_xpm,
    waveform6_xpm
};

static GdkPixmap *waveform_pixmaps[7];

/* This is a dummy pixmap we use when a pixmap can't be found. */
static char *dummy_pixmap_xpm[] = {
/* columns rows colors chars-per-pixel */
"1 1 1 1",
"  c None",
/* pixels */
" "
};

/* This is an internally used function to create pixmaps. */
static GtkWidget*
create_dummy_pixmap                    (GtkWidget       *widget)
{
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
                                                     NULL, dummy_pixmap_xpm);
  if (gdkpixmap == NULL)
    g_error ("Couldn't create replacement pixmap.");
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

/* window must be realized before you call this */
static GtkWidget *
create_halloween_colored_pixmap_from_data(GtkWidget   *window,
                                          const gchar *bitmap_data,
                                          gint         bitmap_width,
                                          gint         bitmap_height)
{
    GdkColormap *colormap;
    GdkColor     fg_color;
    GdkColor     bg_color;
    GdkPixmap   *gdkpixmap;
    GtkWidget   *pixmap;

    colormap = gtk_widget_get_colormap (window);
    fg_color.red = 0;
    fg_color.green = 0;
    fg_color.blue = 0;
    if (!gdk_color_alloc(colormap, &fg_color)) {
        g_warning("couldn't allocate fg color");
    }
    bg_color.red = 0xfcfc;
    bg_color.green = 0xa4a4;
    bg_color.blue = 0;
    if (!gdk_color_alloc(colormap, &bg_color)) {
        g_warning("couldn't allocate bg color");
    }
    gdkpixmap = gdk_pixmap_create_from_data ((GdkWindow *)(window->window),
                                             bitmap_data,
                                             bitmap_width,
                                             bitmap_height,
                                             -1,
                                             &fg_color,
                                             &bg_color);
    if (gdkpixmap == NULL) {
        g_warning("error creating pixmap");
        return create_dummy_pixmap (window);
    }
    pixmap = gtk_pixmap_new (gdkpixmap, NULL);
    gdk_pixmap_unref (gdkpixmap);
    return pixmap;
}

GtkWidget *
create_about_pixmap(GtkWidget *window)
{
    return create_halloween_colored_pixmap_from_data(window,
                                                     bitmap_about_bits,
                                                     bitmap_about_width,
                                                     bitmap_about_height);
}

GtkWidget *
create_logo_pixmap(GtkWidget *window)
{
    return create_halloween_colored_pixmap_from_data(window,
                                                     bitmap_logo_bits,
                                                     bitmap_logo_width,
                                                     bitmap_logo_height);
}

void
create_waveform_gdk_pixmaps(GtkWidget *widget)
{
    GdkColormap *colormap;
    GdkPixmap *gdkpixmap;
    int i;

    colormap = gtk_widget_get_colormap (widget);
    for (i = 0; i <= 6; i++)
        waveform_pixmaps[i] = NULL;

    for (i = 0; i <= 6; i++) {
        gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, NULL,
                                                           NULL, waveform_xpms[i]);
        if (gdkpixmap == NULL) {
            g_warning ("Error creating waveform pixmap %d", i);
            gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, NULL,
                                                               NULL, dummy_pixmap_xpm);
        }
        if (gdkpixmap == NULL) {
            g_error ("Couldn't create replacement pixmap.");
        }
        waveform_pixmaps[i] = gdkpixmap;
    }
}

void
free_waveform_gdk_pixmaps(void)
{
    int i;
    for (i = 0; i <= 6; i++) {
        if (waveform_pixmaps[i] != NULL)
            gdk_pixmap_unref (waveform_pixmaps[i]);
        waveform_pixmaps[i] = NULL;
    }
}

GtkWidget *
create_waveform_pixmap(GtkWidget *widget)
{
    GtkWidget *pixmap;

    pixmap = gtk_pixmap_new (waveform_pixmaps[0], NULL);
    return pixmap;
}

void
set_waveform_pixmap(GtkWidget *widget, int waveform)
{
    if (waveform >=0 && waveform <= 6)
        gtk_pixmap_set(GTK_PIXMAP(widget), waveform_pixmaps[waveform], NULL);
}

