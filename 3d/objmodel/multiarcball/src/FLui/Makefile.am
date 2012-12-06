
INCLUDES = -I../math

AM_CXXFLAGS =

if USE_FLTK 

lib_LTLIBRARIES = libFLarcball.la

libFLarcball_la_CXXFLAGS =  @FL_CXXFLAGS@

libFLarcball_la_SOURCES = FlGlArcballWindow.cpp ../flcursors/fltk1/Fl_Custom_Cursor.cxx

libFLarcball_la_LDFLAGS = @LTNOUNDEF@ @LIB_XCURSOR@ @FL_LDFLAGS@ -version-info @MA_LT_VERSION@ 

libFLarcball_la_LIBADD = ../math/lib3dmath.la 

libFLarcball_la_HEADERS = FlGlArcballWindow.h

libFLarcball_ladir = \
	$(pkgincludedir)/FL

noinst_HEADERS = ../flcursors/fltk1/Fl_Custom_Cursor.H

endif