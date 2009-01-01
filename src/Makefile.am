## Process this file with automake to produce Makefile.in

if BUILD_GUI
ui_PROGRAMS = Xsynth_gtk
else
ui_PROGRAMS =
endif

uidir = $(libdir)/dssi/xsynth-dssi

plugindir = $(libdir)/dssi

plugin_LTLIBRARIES = xsynth-dssi.la

Xsynth_gtk_SOURCES = \
	gui_callbacks.c \
	gui_callbacks.h \
	gui_data.c \
	gui_data.h \
	gui_friendly_patches.c \
	gui_images.c \
	gui_images.h \
	bitmap_about.xbm \
	bitmap_logo.xbm \
	bitmap_waveform0.xpm \
	bitmap_waveform1.xpm \
	bitmap_waveform2.xpm \
	bitmap_waveform3.xpm \
	bitmap_waveform4.xpm \
	bitmap_waveform5.xpm \
	bitmap_waveform6.xpm \
	gui_interface.c \
	gui_interface.h \
	gtkknob.c \
	gtkknob.h \
	gui_main.c \
	gui_main.h \
	xsynth.h \
	xsynth_data.c \
	xsynth_ports.c \
	xsynth_ports.h \
	xsynth_synth.h \
	xsynth_types.h \
	xsynth_voice.h

Xsynth_gtk_CFLAGS = @GTK_CFLAGS@ $(AM_CFLAGS)

Xsynth_gtk_LDADD = -lm @GTK_LIBS@ $(AM_LDFLAGS)

xsynth_dssi_la_SOURCES = \
	gui_data.h \
	gui_friendly_patches.c \
	minblep_tables.c \
	xsynth-dssi.c \
	xsynth.h \
	xsynth_data.c \
	xsynth_ports.c \
	xsynth_ports.h \
	xsynth_synth.c \
	xsynth_synth.h \
	xsynth_types.h \
	xsynth_voice.c \
	xsynth_voice.h \
	xsynth_voice_blosc.h \
	xsynth_voice_render.c

xsynth_dssi_la_LDFLAGS = -module -avoid-version

xsynth_dssi_la_LIBADD = -lm

