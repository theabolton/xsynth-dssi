## Process this file with automake to produce Makefile.in

SUBDIRS = src

EXTRA_DIST = \
	autogen.sh \
	extra/dump_friendly_patches.c \
	extra/dump_old_xsynth_patch.c \
	extra/factory_patches.Xsynth \
	extra/friendly_patches.Xsynth \
	extra/version_0.1_patches.Xsynth \
	src/xsynth_voice_render-original.c

dist_pkgdata_DATA = extra/factory_patches.Xsynth extra/version_0.1_patches.Xsynth

AUTOMAKE_OPTIONS = foreign

