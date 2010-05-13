/*
 * A collection of ugly and random junk brought in from Win32
 * which desparately needs to be tidied up
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "events.h"
#include "uae.h"
#include "autoconf.h"
#include "traps.h"
#include "enforcer.h"
#include "picasso96.h"
#include "driveclick.h"

#define TRUE 1
#define FALSE 0

static int logging_started;
#define LOG_BOOT "puaebootlog.txt"
#define LOG_NORMAL "puaelog.txt"

static int tablet;
static int axmax, aymax, azmax;
static int xmax, ymax, zmax;
static int xres, yres;
static int maxpres;
static TCHAR *tabletname;
static int tablet_x, tablet_y, tablet_z, tablet_pressure, tablet_buttons, tablet_proximity;
static int tablet_ax, tablet_ay, tablet_az, tablet_flags;

unsigned int log_scsi = 1;
int log_net, uaelib_debug;
unsigned int flashscreen;

struct winuae_currentmode {
        unsigned int flags;
        int native_width, native_height, native_depth, pitch;
        int current_width, current_height, current_depth;
        int amiga_width, amiga_height;
        int frequency;
        int initdone;
        int fullfill;
        int vsync;
};

static struct winuae_currentmode currentmodestruct;
static struct winuae_currentmode *currentmode = &currentmodestruct;

static int serial_period_hsyncs, serial_period_hsync_counter;
static int data_in_serdatr; /* new data received */

// --- dinput.cpp START ---
int rawkeyboard = -1;
int no_rawinput;
// --- dinput.cpp END -----

static uae_u32 REGPARAM2 gfxmem_lgetx (uaecptr addr)
{
	uae_u32 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u32 *)(gfxmemory + addr);
	return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 gfxmem_wgetx (uaecptr addr)
{
	uae_u16 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u16 *)(gfxmemory + addr);
	return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 gfxmem_bgetx (uaecptr addr)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return gfxmemory[addr];
}

static void REGPARAM2 gfxmem_lputx (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u32 *)(gfxmemory + addr);
	do_put_mem_long (m, l);
}

static void REGPARAM2 gfxmem_wputx (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u16 *)(gfxmemory + addr);
	do_put_mem_word (m, (uae_u16)w);
}

static void REGPARAM2 gfxmem_bputx (uaecptr addr, uae_u32 b)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	gfxmemory[addr] = b;
}

static int REGPARAM2 gfxmem_check (uaecptr addr, uae_u32 size)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return (addr + size) < allocated_gfxmem;
}

static uae_u8 *REGPARAM2 gfxmem_xlate (uaecptr addr)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return gfxmemory + addr;
}


addrbank gfxmem_bankx = {
	gfxmem_lgetx, gfxmem_wgetx, gfxmem_bgetx,
	gfxmem_lputx, gfxmem_wputx, gfxmem_bputx,
	gfxmem_xlate, gfxmem_check, NULL, "RTG RAM",
	dummy_lgeti, dummy_wgeti, ABFLAG_RAM
};

void getgfxoffset (int *dxp, int *dyp, int *mxp, int *myp)
{
*dxp = 0;
*dyp = 0;
*mxp = 0;
*myp = 0;
/*
        int dx, dy;

        getfilteroffset (&dx, &dy, mxp, myp);
        *dxp = dx;
        *dyp = dy;
        if (picasso_on) {
                dx = picasso_offset_x;
                dy = picasso_offset_y;
                *mxp = picasso_offset_mx;
                *myp = picasso_offset_my;
        }
        *dxp = dx;
        *dyp = dy;
        if (currentmode->flags & DM_W_FULLSCREEN) {
                if (scalepicasso && screen_is_picasso)
                        return;
                if (usedfilter && !screen_is_picasso)
                        return;
                if (currentmode->fullfill && (currentmode->current_width > currentmode->native_width || currentmode->current_height > currentmode->native_height))
                        return;
                dx += (currentmode->native_width - currentmode->current_width) / 2;
                dy += (currentmode->native_height - currentmode->current_height) / 2;
        }
        *dxp = dx;
        *dyp = dy;
*/
}


int is_tablet (void)
{
        return tablet ? 1 : 0;
}

int vsync_switchmode (int hz, int oldhz)
{
        static int tempvsync;
        int w = currentmode->native_width;
        int h = currentmode->native_height;
        int d = currentmode->native_depth / 8;
//        struct MultiDisplay *md = getdisplay (&currprefs);
        struct PicassoResolution *found;

        int newh, i, cnt;
        int dbl = getvsyncrate (currprefs.chipset_refreshrate) != currprefs.chipset_refreshrate ? 2 : 1;

        if (hz < 0)
                return tempvsync;

        newh = h * oldhz / hz;
        hz = hz * dbl;

        found = NULL;
/*        for (i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
                struct PicassoResolution *r = &md->DisplayModes[i];
                if (r->res.width == w && r->res.height == h && r->depth == d) {
                        int j;
                        for (j = 0; r->refresh[j] > 0; j++) {
                                if (r->refresh[j] == oldhz) {
                                        found = r;
                                        break;
                                }
                        }
                }
        }*/
        if (found == NULL) {
                write_log ("refresh rate changed to %d but original rate was not found\n", hz);
                return 0;
        }

        found = NULL;
/*        for (cnt = 0; cnt <= abs (newh - h) + 1 && !found; cnt++) {
                for (i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
                        struct PicassoResolution *r = &md->DisplayModes[i];
                        if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt) && r->depth == d) {
                                int j;
                                for (j = 0; r->refresh[j] > 0; j++) {
                                        if (r->refresh[j] == hz) {
                                                found = r;
                                                break;
                                        }
                                }
                        }
                }
        }*/
        if (!found) {
                tempvsync = currprefs.gfx_avsync;
                changed_prefs.gfx_avsync = 0;
                write_log ("refresh rate changed to %d but no matching screenmode found, vsync disabled\n", hz);
        } else {
                newh = found->res.height;
                changed_prefs.gfx_size_fs.height = newh;
                changed_prefs.gfx_refreshrate = hz;
                write_log ("refresh rate changed to %d, new screenmode %dx%d\n", hz, w, newh);
        }
/*
        reopen (1);
*/
        return 0;
}

void serial_check_irq (void)
{
        if (data_in_serdatr)
                INTREQ_0 (0x8000 | 0x0800);
}

void serial_uartbreak (int v)
{
#ifdef SERIAL_PORT
        serialuartbreak (v);
#endif
}

void serial_hsynchandler (void)
{
#ifdef AHI
        extern void hsyncstuff(void);
        hsyncstuff();
#endif
/*
        if (serial_period_hsyncs == 0)
                return;
        serial_period_hsync_counter++;
        if (serial_period_hsyncs == 1 || (serial_period_hsync_counter % (serial_period_hsyncs - 1)) == 0) {
                checkreceive_serial (0);
                checkreceive_enet (0);
        }
        if ((serial_period_hsync_counter % serial_period_hsyncs) == 0)
                checksend (0);
*/
}

/*
static int drvsampleres[] = {
        IDR_DRIVE_CLICK_A500_1, DS_CLICK,
        IDR_DRIVE_SPIN_A500_1, DS_SPIN,
        IDR_DRIVE_SPINND_A500_1, DS_SPINND,
        IDR_DRIVE_STARTUP_A500_1, DS_START,
        IDR_DRIVE_SNATCH_A500_1, DS_SNATCH,
        -1
};
*/
int driveclick_loadresource (struct drvsample *sp, int drivetype)
{
/*
        int i, ok;

        ok = 1;
        for (i = 0; drvsampleres[i] >= 0; i += 2) {
                struct drvsample *s = sp + drvsampleres[i + 1];
                HRSRC res = FindResource (NULL, MAKEINTRESOURCE (drvsampleres[i + 0]), "WAVE");
                if (res != 0) {
                        HANDLE h = LoadResource (NULL, res);
                        int len = SizeofResource (NULL, res);
                        uae_u8 *p = LockResource (h);
                        s->p = decodewav (p, &len);
                        s->len = len;
                } else {
                        ok = 0;
                }
        }
        return ok;
*/
}

void driveclick_fdrawcmd_close(int drive)
{
/*
        if (h[drive] != INVALID_HANDLE_VALUE)
                CloseHandle(h[drive]);
        h[drive] = INVALID_HANDLE_VALUE;
        motors[drive] = 0;
*/
}

static int driveclick_fdrawcmd_open_2(int drive)
{
/*
        TCHAR s[32];

        driveclick_fdrawcmd_close(drive);
        _stprintf (s, "\\\\.\\fdraw%d", drive);
        h[drive] = CreateFile(s, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h[drive] == INVALID_HANDLE_VALUE)
                return 0;
        return 1;
*/
}

int driveclick_fdrawcmd_open(int drive)
{
/*
        if (!driveclick_fdrawcmd_open_2(drive))
                return 0;
        driveclick_fdrawcmd_init(drive);
        return 1;
*/
}

void driveclick_fdrawcmd_detect(void)
{
/*
        static int detected;
        if (detected)
                return;
        detected = 1;
        if (driveclick_fdrawcmd_open_2(0))
                driveclick_pcdrivemask |= 1;
        driveclick_fdrawcmd_close(0);
        if (driveclick_fdrawcmd_open_2(1))
                driveclick_pcdrivemask |= 2;
        driveclick_fdrawcmd_close(1);
*/
}

void driveclick_fdrawcmd_seek(int drive, int cyl)
{
//        write_comm_pipe_int (dc_pipe, (drive << 8) | cyl, 1);
}
void driveclick_fdrawcmd_motor (int drive, int running)
{
//        write_comm_pipe_int (dc_pipe, 0x8000 | (drive << 8) | (running ? 1 : 0), 1);
}

void driveclick_fdrawcmd_vsync(void)
{
/*
        int i;
        for (i = 0; i < 2; i++) {
                if (motors[i] > 0) {
                        motors[i]--;
                        if (motors[i] == 0)
                                CmdMotor(h[i], 0);
                }
        }
*/
}

static int driveclick_fdrawcmd_init(int drive)
{
/*
        static int thread_ok;

        if (h[drive] == INVALID_HANDLE_VALUE)
                return 0;
        motors[drive] = 0;
        SetDataRate(h[drive], 3);
        CmdSpecify(h[drive], 0xd, 0xf, 0x1, 0);
        SetMotorDelay(h[drive], 0);
        CmdMotor(h[drive], 0);
        if (thread_ok)
                return 1;
        thread_ok = 1;
        init_comm_pipe (dc_pipe, DC_PIPE_SIZE, 3);
        uae_start_thread ("DriveClick", driveclick_thread, NULL, NULL);
        return 1;
*/
}

uae_u32 emulib_target_getcpurate (uae_u32 v, uae_u32 *low)
{
/*
        *low = 0;
        if (v == 1) {
                LARGE_INTEGER pf;
                pf.QuadPart = 0;
                QueryPerformanceFrequency (&pf);
                *low = pf.LowPart;
                return pf.HighPart;
        } else if (v == 2) {
                LARGE_INTEGER pf;
                pf.QuadPart = 0;
                QueryPerformanceCounter (&pf);
                *low = pf.LowPart;
                return pf.HighPart;
        }
*/
        return 0;
}

int isfat (uae_u8 *p)
{
	int i, b;

	if ((p[0x15] & 0xf0) != 0xf0)
		return 0;
	if (p[0x0b] != 0x00 || p[0x0c] != 0x02)
		return 0;
	b = 0;
	for (i = 0; i < 8; i++) {
		if (p[0x0d] & (1 << i))
			b++;
	}
	if (b != 1)
		return 0;
	if (p[0x0f] != 0)
		return 0;
	if (p[0x0e] > 8 || p[0x0e] == 0)
		return 0;
	if (p[0x10] == 0 || p[0x10] > 8)
		return 0;
	b = (p[0x12] << 8) | p[0x11];
	if (b > 8192 || b <= 0)
		return 0;
	b = p[0x16] | (p[0x17] << 8);
	if (b == 0 || b > 8192)
		return 0;
	return 1;
}

void setmouseactivexy (int x, int y, int dir)
{
/*        int diff = 8;

        if (isfullscreen () > 0)
                return;
        x += amigawin_rect.left;
        y += amigawin_rect.top;
        if (dir & 1)
                x = amigawin_rect.left - diff;
        if (dir & 2)
                x = amigawin_rect.right + diff;
        if (dir & 4)
                y = amigawin_rect.top - diff;
        if (dir & 8)
                y = amigawin_rect.bottom + diff;
        if (!dir) {
                x += (amigawin_rect.right - amigawin_rect.left) / 2;
                y += (amigawin_rect.bottom - amigawin_rect.top) / 2;
        }
        if (mouseactive) {
                disablecapture ();
                SetCursorPos (x, y);
                if (dir)
                        recapture = 1;
        }*/
}

void setmouseactive (int active)
{
}

char *au_fs_copy (char *dst, int maxlen, const char *src)
{
        int i;

        for (i = 0; src[i] && i < maxlen - 1; i++)
                dst[i] = src[i];
        dst[i] = 0;
        return dst;
}

int my_existsfile (const char *name)
{
		struct stat sonuc;
		if (lstat (name, &sonuc) == -1) {
			return 0;
		} else {
			if (!S_ISDIR(sonuc.st_mode))
				return 1;
		}
        return 0;
}

int my_existsdir (const char *name)
{
		struct stat sonuc;

		if (lstat (name, &sonuc) == -1) {
			return 0;
		} else {
			if (S_ISDIR(sonuc.st_mode))
				return 1;
		}
        return 0;
}

int my_getvolumeinfo (const char *root)
{
		struct stat sonuc;
        int ret = 0;

        if (lstat (root, &sonuc) == -1)
                return -1;
        if (!S_ISDIR(sonuc.st_mode))
                return -1;
//------------
        return ret;
}

// --- clipboard.c --- temporary here ---
static uaecptr clipboard_data;
static int vdelay, signaling, initialized;

void amiga_clipboard_die (void)
{
	signaling = 0;
	write_log ("clipboard not initialized\n");
}

void amiga_clipboard_init (void)
{
	signaling = 0;
	write_log ("clipboard initialized\n");
	initialized = 1;
}

void amiga_clipboard_task_start (uaecptr data)
{
	clipboard_data = data;
	signaling = 1;
	write_log ("clipboard task init: %08x\n", clipboard_data);
}

uae_u32 amiga_clipboard_proc_start (void)
{
	write_log ("clipboard process init: %08x\n", clipboard_data);
	signaling = 1;
	return clipboard_data;
}

void amiga_clipboard_got_data (uaecptr data, uae_u32 size, uae_u32 actual)
{
	uae_u8 *addr;
	if (!initialized) {
		write_log ("clipboard: got_data() before initialized!?\n");
		return;
	}
}


int get_guid_target (uae_u8 *out)
{
	unsigned Data1, Data2, Data3, Data4;

	srand(time(NULL));
	Data1 = rand();
	Data2 = ((rand() & 0x0fff) | 0x4000);
	Data3 = rand() % 0x3fff + 0x8000;
	Data4 = rand();

	out[0] = Data1 >> 24;
	out[1] = Data1 >> 16;
	out[2] = Data1 >>  8;
	out[3] = Data1 >>  0;
	out[4] = Data2 >>  8;
	out[5] = Data2 >>  0;
	out[6] = Data3 >>  8;
	out[7] = Data3 >>  0;
	memcpy (out + 8, Data4, 8);
	return 1;
}

static int testwritewatch (void)
{
}

void machdep_free (void)
{
#ifdef LOGITECHLCD
        lcd_close ();
#endif
}

void target_run (void)
{
        //shellexecute (currprefs.win32_commandpathstart);
}

void fetch_path (char *name, char *out, int size)
{
        int size2 = size;

        _tcscpy (out, start_path_data);
        if (!name)
                return;
}
void fetch_configurationpath (char *out, int size)
{
        fetch_path ("ConfigurationPath", out, size);
}
void fetch_screenshotpath (char *out, int size)
{
        fetch_path ("ScreenshotPath", out, size);
}
void fetch_ripperpath (char *out, int size)
{
        fetch_path ("RipperPath", out, size);
}
void fetch_datapath (char *out, int size)
{
        fetch_path (NULL, out, size);
}

// --- dinput.cpp ---
int input_get_default_keyboard (int i)
{
        if (rawkeyboard > 0) {
                if (i == 0)
                        return 0;
                return 1;
        } else {
                if (i == 0)
                        return 1;
                return 0;
        }
}

// --- unicode.cpp ---
static unsigned int fscodepage;

char *ua_fs (const char *s, int defchar)
{
	return s;
}

char *ua_copy (char *dst, int maxlen, const char *src)
{
        dst[0] = 0;
		strncpy (dst, src, maxlen);
        return dst;
}

// --- win32gui.cpp ---
static int qs_override;

int target_cfgfile_load (struct uae_prefs *p, char *filename, int type, int isdefault)
{
	int v, i, type2;
	int ct, ct2 = 0, size;
	char tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	char fname[MAX_DPATH];

	_tcscpy (fname, filename);
	if (!zfile_exists (fname)) {
		fetch_configurationpath (fname, sizeof (fname) / sizeof (TCHAR));
		if (_tcsncmp (fname, filename, _tcslen (fname)))
			_tcscat (fname, filename);
		else
			_tcscpy (fname, filename);
	}

	if (!isdefault)
		qs_override = 1;
	if (type < 0) {
		type = 0;
		cfgfile_get_description (fname, NULL, NULL, NULL, &type);
	}
	if (type == 0 || type == 1) {
		discard_prefs (p, 0);
	}
	type2 = type;
	if (type == 0) {
		default_prefs (p, type);
	}
		
	//regqueryint (NULL, "ConfigFile_NoAuto", &ct2);
	v = cfgfile_load (p, fname, &type2, ct2, isdefault ? 0 : 1);
	if (!v)
		return v;
	if (type > 0)
		return v;
	for (i = 1; i <= 2; i++) {
		if (type != i) {
			size = sizeof (ct);
			ct = 0;
			//regqueryint (NULL, configreg2[i], &ct);
			if (ct && ((i == 1 && p->config_hardware_path[0] == 0) || (i == 2 && p->config_host_path[0] == 0) || ct2)) {
				size = sizeof (tmp1) / sizeof (TCHAR);
				//regquerystr (NULL, configreg[i], tmp1, &size);
				fetch_path ("ConfigurationPath", tmp2, sizeof (tmp2) / sizeof (TCHAR));
				_tcscat (tmp2, tmp1);
				v = i;
				cfgfile_load (p, tmp2, &v, 1, 0);
			}
		}
	}
	v = 1;
	return v;
}
// --- win32gfx.c
int screen_is_picasso = 0;
struct uae_filter *usedfilter;
uae_u32 redc[3 * 256], grec[3 * 256], bluc[3 * 256];

static int isfullscreen_2 (struct uae_prefs *p)
{
        if (screen_is_picasso)
                return p->gfx_pfullscreen == 1 ? 1 : (p->gfx_pfullscreen == 2 ? -1 : 0);
        else
                return p->gfx_afullscreen == 1 ? 1 : (p->gfx_afullscreen == 2 ? -1 : 0);
}
int isfullscreen (void)
{
        return isfullscreen_2 (&currprefs);
}

// --- win32.c
uae_u8 *save_log (int bootlog, int *len)
{
        FILE *f;
        uae_u8 *dst = NULL;
        int size;

        if (!logging_started)
                return NULL;
        f = _tfopen (bootlog ? LOG_BOOT : LOG_NORMAL, "rb");
        if (!f)
                return NULL;
        fseek (f, 0, SEEK_END);
        size = ftell (f);
        fseek (f, 0, SEEK_SET);
        if (size > 30000)
                size = 30000;
        if (size > 0) {
                dst = xcalloc (uae_u8, size + 1);
                if (dst)
                        fread (dst, 1, size, f);
                fclose (f);
                *len = size + 1;
        }
        return dst;
}
