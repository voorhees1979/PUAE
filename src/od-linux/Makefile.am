AM_CPPFLAGS  = @UAE_CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS    = @UAE_CFLAGS@

noinst_LIBRARIES = libosdep.a

libosdep_a_SOURCES = main.c memory.c blkdev-linux.c

noinst_HEADERS = memory.h hrtimer.h
