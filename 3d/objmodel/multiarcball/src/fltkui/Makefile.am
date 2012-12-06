
INCLUDES = -I../math

AM_CXXFLAGS = 

if USE_FLTK2

lib_LTLIBRARIES = libfltkarcball.la

libfltkarcball_la_CXXFLAGS =  @FL2_CXXFLAGS@

libfltkarcball_la_SOURCES = GlArcballWindow.cpp 
#../misc/CustomCursorShape.cxx

libfltkarcball_la_LDFLAGS = @LTNOUNDEF@ @FL2_LDFLAGS@ -version-info @MA_LT_VERSION@

libfltkarcball_la_LIBADD = ../math/lib3dmath.la

libfltkarcball_la_HEADERS = GlArcballWindow.h

libfltkarcball_ladir = \
	$(pkgincludedir)/fltk

#noinst_HEADERS =  ../misc/CustomCursorShape.h

endif