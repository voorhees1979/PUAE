AM_CPPFLAGS  = @UAE_CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS    = @UAE_CFLAGS@
AM_CFLAGS   += @X_CFLAGS@

noinst_LIBRARIES = libgfxdep.a

libgfxdep_a_SOURCES = xwin.c x11keys.c

noinst_HEADERS = gfx.h
