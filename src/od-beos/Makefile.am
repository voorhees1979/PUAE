AM_CPPFLAGS  = @UAE_CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS    = @UAE_CFLAGS@
AM_CXXFLAGS  = @UAE_CFLAGS@ -Wno-multichar

noinst_LIBRARIES = libosdep.a

libosdep_a_SOURCES  = main.cpp memory.c

noinst_HEADERS = memory.h hrtimer.h

dist_noinst_DATA = uae.rsrc
