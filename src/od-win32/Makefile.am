AM_CPPFLAGS  = @UAE_CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS    = @UAE_CFLAGS@

noinst_LIBRARIES = libosdep.a

libosdep_a_SOURCES = main.c memory.c posixemu.c writelog.c fsdb_win32.c \
		     hardfile_win32.c bsdsock.c

noinst_HEADERS = memory.h hrtimer.h posixemu.h
