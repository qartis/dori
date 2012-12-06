SUBDIRS=. src test vc

if USE_FLTK
flpc=multiarcball-FL.pc
endif
if USE_FLTK2
fl2pc=multiarcball-fltk2.pc
endif
if USE_GTK
mmpc=multiarcballmm.pc
endif


pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= multiarcball.pc $(flpc) $(fl2pc) $(mmpc)

noinst_DATA=TODO Doxyfile

multiarcball_docdir = $(docdir)

multiarcball_doc_DATA= doc/eventsmap.html \
		doc/fivepercent.png

EXTRA_DIST = $(multiarcball_doc_DATA) $(noinst_DATA)