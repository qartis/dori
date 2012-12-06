
INCLUDES = -I../src/math

AM_CXXFLAGS =

if USE_FLTK 

testfl = testfl1

testfl1_CXXFLAGS =  @FL_CXXFLAGS@ -I../src/FLui

testfl1_SOURCES = testFL.cpp

testfl1_LDFLAGS = ../src/FLui/libFLarcball.la @FL_LDFLAGS@ @LIB_XCURSOR@

testfl1_LDADD = 

testfl1ex = examplefl1

examplefl1_CXXFLAGS =  @FL_CXXFLAGS@ -I../src/FLui

examplefl1_SOURCES = exampleFL.cpp

examplefl1_LDFLAGS = ../src/FLui/libFLarcball.la @FL_LDFLAGS@ @LIB_XCURSOR@

examplefl1_LDADD = 

endif
if USE_FLTK2
testfltk = testfl2

testfl2_CXXFLAGS =  @FL2_CXXFLAGS@ -I../src/fltkui

testfl2_SOURCES = testFL2.cpp

testfl2_LDFLAGS = ../src/fltkui/libfltkarcball.la @FL2_LDFLAGS@

testfl2_LDADD = 


endif
if USE_GTK

testmm = testgtkmm

testgtkmm_CXXFLAGS =   @GTKGLEXTMM_CFLAGS@ -I../src/gtkmmui

testgtkmm_SOURCES = testGtkmm.cxx

testgtkmm_LDFLAGS =  ../src/gtkmmui/libgtkmmarcball.la @GTKGLEXTMM_LIBS@ 

testgtkmm_LDADD =

exmm = examplegtkmm

examplegtkmm_CXXFLAGS =   @GTKGLEXTMM_CFLAGS@ -I../src/gtkmmui

examplegtkmm_SOURCES = exampleGtkmm.cxx

examplegtkmm_LDFLAGS =  ../src/gtkmmui/libgtkmmarcball.la @GTKGLEXTMM_LIBS@

examplegtkmm_LDADD = 

endif

testmath = testmath

testmath_CXXFLAGS = 

testmath_SOURCES = testmath.cpp

testmath_LDFLAGS = 

testmath_LDADD = 
 

noinst_PROGRAMS= ${testmath} ${testfl} ${testfl1ex} ${testfltk} ${testmm} ${exmm} 

noinst_HEADERS= draw_cube.h

all: ${test_PROGRAMS}
