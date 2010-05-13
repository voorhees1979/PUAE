 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "gui.h"

int gui_init (void)
{
}

int gui_open (void)
{
    return -1;
}

void gui_notify_state (int state)
{
}

void gui_fps (int fps, int idle)
{
    gui_data.fps  = fps;
    gui_data.idle = idle;
}

void gui_flicker_led (int led, int unitnum, int status)
{
}

void gui_led (int led, int on)
{
}

void gui_hd_led (int unitnum, int led)
{
}

void gui_cd_led (int unitnum, int led)
{
}

void gui_filename (int num, const char *name)
{
}

void gui_handle_events (void)
{
}

int gui_update (void)
{
	return 0;
}

void gui_exit (void)
{
}

void gui_display(int shortcut)
{
}

void gui_message (const char *format,...)
{
       char msg[2048];
       va_list parms;

       va_start (parms,format);
       vsprintf ( msg, format, parms);
       va_end (parms);

       write_log (msg);
}

void gui_disk_image_change (int unitnum, const TCHAR *name) {}
void gui_lock (void) {}
void gui_unlock (void) {}

