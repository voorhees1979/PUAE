#ifndef EVENTS_H
#define EVENTS_H

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Events
  * These are best for low-frequency events. Having too many of them,
  * or using them for events that occur too frequently, can cause massive
  * slowdown.
  *
  * Copyright 1995-1998 Bernd Schmidt
  */

#undef EVENT_DEBUG

#include "machdep/rpt.h"
#include "hrtimer.h"

/* Every Amiga hardware clock cycle takes this many "virtual" cycles.  This
 * used to be hardcoded as 1, but using higher values allows us to time some
 * stuff more precisely.
 * 512 is the official value from now on - it can't change, unless we want
 * _another_ config option "finegrain2_m68k_speed".
 *
 * We define this value here rather than in events.h so that gencpu.c sees
 * it.
 */
#define CYCLE_UNIT 512

/* This one is used by cfgfile.c.  We could reduce the CYCLE_UNIT back to 1,
 * I'm not 100% sure this code is bug free yet.
 */
#define OFFICIAL_CYCLE_UNIT 512

extern volatile frame_time_t vsynctime, vsyncmintime;
extern void reset_frame_rate_hack (void);
extern int rpt_available;
extern frame_time_t syncbase;

extern void compute_vsynctime (void);
extern void init_eventtab (void);
extern void do_cycles_ce (long cycles);
extern void do_cycles_ce020 (int clocks);
extern void do_cycles_ce020_mem (int clocks);
extern void do_cycles_ce000 (int clocks);
extern int is_cycle_ce (void);

extern unsigned long currcycle, nextevent, is_lastline;
typedef void (*evfunc)(void);
typedef void (*evfunc2)(uae_u32);

typedef unsigned long int evt;

struct ev
{
    int active;
    evt evtime, oldcycles;
    evfunc handler;
};

struct ev2
{
    int active;
    evt evtime;
    uae_u32 data;
    evfunc2 handler;
};

enum {
    ev_hsync, ev_audio, ev_cia, ev_misc,
    ev_max
};

enum {
    ev2_blitter, ev2_disk, ev2_misc,
    ev2_max = 12
};

extern struct ev eventtab[ev_max];
extern struct ev2 eventtab2[ev2_max];

extern void event2_newevent(int, evt);
extern void event2_newevent2(evt, uae_u32, evfunc2);
extern void event2_remevent(int);

#if 0
#ifdef JIT
#include "events_jit.h"
#else
#include "events_normal.h"
#endif
#else
#include "events_jit.h"
#endif

STATIC_INLINE int current_hpos (void)
{
    return (get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT;
}

#endif
