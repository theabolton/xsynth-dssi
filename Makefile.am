## Process this file with automake to produce Makefile.in

SUBDIRS = src

EXTRA_DIST = \
	autogen.sh \
	extra/dump_friendly_patches.c \
	extra/dump_old_xsynth_patch.c \
	extra/factory_patches.Xsynth \
	extra/patch_library.Xsynth \
	src/xsynth_voice_render-original.c

dist_pkgdata_DATA = extra/factory_patches.Xsynth extra/patch_library.Xsynth

AUTOMAKE_OPTIONS = foreign

##X install-data-local:
##X 	@$(NORMAL_INSTALL)
##X 	if test -d $(srcdir)/pixmaps; then \
##X 	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
##X 	  for pixmap in $(srcdir)/pixmaps/*; do \
##X 	    if test -f $$pixmap; then \
##X 	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
##X 	    fi \
##X 	  done \
##X 	fi
##X 
##X dist-hook:
##X 	if test -d pixmaps; then \
##X 	  mkdir $(distdir)/pixmaps; \
##X 	  for pixmap in pixmaps/*; do \
##X 	    if test -f $$pixmap; then \
##X 	      cp -p $$pixmap $(distdir)/pixmaps; \
##X 	    fi \
##X 	  done \
##X 	fi

