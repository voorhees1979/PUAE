 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Prototypes for main.c
  *
  * Copyright 1996 Bernd Schmidt
  * Copyright 2006-2007 Richard Drummond
  */

extern void do_start_program (void);
extern void do_leave_program (void);
extern void start_program (void);
extern void leave_program (void);
extern void real_main (int, char **);
extern void usage (void);

extern void sleep_millis (int ms);
extern void sleep_millis_busy (int ms);
extern int sleep_resolution;

extern void uae_reset (int);
extern void uae_quit (void);
extern void uae_restart (int, char*);
extern void reset_all_systems (void);
extern void target_reset (void);
extern void target_addtorecent (const char*, int);
extern void target_run (void);
extern void target_quit (void);

extern int quit_program;
extern int console_emulation;

extern char warning_buffer[256];
extern char start_path_data[];
extern char start_path_data_exe[];

extern void setup_brkhandler (void);

#ifdef USE_SDL
int init_sdl (void);
#endif

#define UAE_STATE_STOPPED    0
#define UAE_STATE_RUNNING    1
#define UAE_STATE_PAUSED     2
#define UAE_STATE_COLD_START 3
#define UAE_STATE_WARM_START 4
#define UAE_STATE_QUITTING   5

int uae_get_state (void);
int uae_state_change_pending (void);
