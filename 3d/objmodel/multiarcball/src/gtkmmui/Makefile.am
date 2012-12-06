
INCLUDES = -I../math

AM_CXXFLAGS =

if USE_GTK

lib_LTLIBRARIES = libgtkmmarcball.la

libgtkmmarcball_la_CXXFLAGS =  @GTKGLEXTMM_CFLAGS@ @PANGOFT2_CFLAGS@

libgtkmmarcball_la_SOURCES = gtkmm_arcball.cpp

libgtkmmarcball_la_LDFLAGS = @LTNOUNDEF@ @GTKGLEXTMM_LIBS@ -version-info @MA_LT_VERSION@

libgtkmmarcball_la_LIBADD = ../math/lib3dmath.la

libgtkmmarcball_la_HEADERS = gtkmm_arcball.h

libgtkmmarcball_ladir = \
	$(pkgincludedir)


endif
