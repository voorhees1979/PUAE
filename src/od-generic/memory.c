 /*
  * UAE - The Un*x Amiga Emulator
  *
  * OS-specific memory support functions
  *
  * Copyright 2004 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include <sys/sysctl.h>
#include "include/memory.h"

#ifdef JIT

#define BARRIER 32
#define MAXZ3MEM 0x7F000000
#define MAXZ3MEM64 0xF0000000
#define MAX_SHMID 256

#define IPC_PRIVATE 0x01
#define IPC_RMID    0x02
#define IPC_CREAT   0x04
#define IPC_STAT    0x08

typedef int key_t;

/* One shmid data structure for each shared memory segment in the system. */
struct shmid_ds {
    key_t  key;
    size_t size;
    void   *addr;
    char  name[MAX_PATH];
    void   *attached;
    int    mode;
    void   *natmembase;
};

static struct shmid_ds shmids[MAX_SHMID];
static int memwatchok = 0;
uae_u8 *natmem_offset, *natmem_offset_end;
static uae_u8 *p96mem_offset;
static int p96mem_size;
static uae_u8 *memwatchtable;
static uae_u64 size64;
int maxmem;

#include <sys/mman.h>

/*
 * Allocate executable memory for JIT cache
 */
uae_u8 *cache_alloc (int size)
{
   void *cache;

   size = size < getpagesize() ? getpagesize() : size;

	if ((cache = valloc (size)))
		mprotect (cache, size, PROT_READ|PROT_WRITE|PROT_EXEC);

   return cache;
}

void cache_free (uae_u8 *cache)
{
    free (cache);
}

#ifdef NATMEM_OFFSET
static uae_u32 lowmem (void)
{
	uae_u32 change = 0;
	if (currprefs.z3fastmem_size + currprefs.z3fastmem2_size >= 8 * 1024 * 1024) {
		if (currprefs.z3fastmem2_size) {
			if (currprefs.z3fastmem2_size <= 128 * 1024 * 1024) {
				change = currprefs.z3fastmem2_size;
				currprefs.z3fastmem2_size = 0;
			} else {
				change = currprefs.z3fastmem2_size / 2;
				currprefs.z3fastmem2_size >>= 1;
				changed_prefs.z3fastmem2_size = currprefs.z3fastmem2_size;
			}
		} else {
			change = currprefs.z3fastmem_size - currprefs.z3fastmem_size / 4;
			currprefs.z3fastmem2_size = changed_prefs.z3fastmem2_size = currprefs.z3fastmem_size / 4;
			currprefs.z3fastmem_size >>= 1;
			changed_prefs.z3fastmem_size = currprefs.z3fastmem_size;
		}
	} else if (currprefs.gfxmem_size >= 1 * 1024 * 1024) {
		change = currprefs.gfxmem_size - currprefs.gfxmem_size / 2;
		currprefs.gfxmem_size >>= 1;
		changed_prefs.gfxmem_size = currprefs.gfxmem_size;
	}
	if (currprefs.z3fastmem2_size < 128 * 1024 * 1024)
		currprefs.z3fastmem2_size = changed_prefs.z3fastmem2_size = 0;
	return change;
}

void preinit_shm (void)
{
	int i;
	uae_u64 total64;
	uae_u64 totalphys64;
	uae_u32 max_allowed_mman;

#ifdef CPU_64_BIT
	max_allowed_mman = 2048;
#else
	max_allowed_mman = 1536;
#endif

#ifdef __APPLE__
//xaind
	int mib[2];
	size_t len;
        
	mib[0] = CTL_HW;
	mib[1] = HW_MEMSIZE; /* gives a 64 bit int */
	len = sizeof(totalphys64);
	sysctl(mib, 2, &totalphys64, &len, NULL, 0);
	total64 = (uae_u64) totalphys64;
#else
	totalphys64 = sysconf (_SC_PHYS_PAGES) * getpagesize();
	total64 = (uae_u64)sysconf (_SC_PHYS_PAGES) * (uae_u64)getpagesize();
#endif
	size64 = total64;
	if (maxmem < 0)
		size64 = MAXZ3MEM;
	else if (maxmem > 0)
		size64 = maxmem * 1024 * 1024;
#ifdef CPU_64_BIT
	if (size64 > MAXZ3MEM64)
		size64 = MAXZ3MEM64;
#else
	if (size64 > MAXZ3MEM)
		size64 = MAXZ3MEM;
#endif
	if (size64 < 8 * 1024 * 1024)
		size64 = 8 * 1024 * 1024;
	if (max_allowed_mman * 1024 * 1024 > size64)
		max_allowed_mman = size64 / (1024 * 1024);
	max_z3fastmem = max_allowed_mman * 1024 * 1024;
	if (max_z3fastmem < 512 * 1024 * 1024)
		max_z3fastmem = 512 * 1024 * 1024;

	shm_start = 0;
	for (i = 0; i < MAX_SHMID; i++) {
		shmids[i].attached = 0;
		shmids[i].key = -1;
		shmids[i].size = 0;
		shmids[i].addr = NULL;
		shmids[i].name[0] = 0;
	}

	write_log ("Max Z3FastRAM %dM. Total physical RAM %u %uM\n", max_z3fastmem >> 20, totalphys64 >> 20, totalphys64 >> 20);
	//testwritewatch ();
	canbang = 1;
}

static void resetmem (void)
{
return;
	int i;

	if (!shm_start)
		return;
	for (i = 0; i < MAX_SHMID; i++) {
		struct shmid_ds *s = &shmids[i];
		int size = s->size;
		uae_u8 *shmaddr;
		uae_u8 *result;

		if (!s->attached)
			continue;
		if (!s->natmembase)
			continue;
		shmaddr = natmem_offset + ((uae_u8*)s->attached - (uae_u8*)s->natmembase);
		result = valloc (/*shmaddr,*/ size);
		if (result != shmaddr)
			write_log ("NATMEM: realloc(%p,%d,%d) failed, err=%x\n", shmaddr, size, s->mode, errno);
		else
			write_log ("NATMEM: rellocated(%p,%d,%s)\n", shmaddr, size, s->name);
	}
}

int init_shm (void)
{
        uae_u32 size, totalsize, z3size, natmemsize, rtgbarrier, rtgextra;
        int rounds = 0;

restart:
        for (;;) {
                int lowround = 0;
                uae_u8 *blah = NULL;
                if (rounds > 0)
                        write_log ("NATMEM: retrying %d..\n", rounds);
                rounds++;
                if (natmem_offset)
                        free(natmem_offset);
                natmem_offset = NULL;
                natmem_offset_end = NULL;
                canbang = 0;

                z3size = 0;
                size = 0x1000000;
                rtgextra = 0;
                rtgbarrier = getpagesize();
                if (currprefs.cpu_model >= 68020)
                        size = 0x10000000;
                if (currprefs.z3fastmem_size || currprefs.z3fastmem2_size) {
                        z3size = currprefs.z3fastmem_size + currprefs.z3fastmem2_size + (currprefs.z3fastmem_start - 0x10000000);
                        if (currprefs.gfxmem_size)
                                rtgbarrier = 16 * 1024 * 1024;
                } else {
                        rtgbarrier = 0;
                }
                totalsize = size + z3size + currprefs.gfxmem_size;
                while (totalsize > size64) {
                        int change = lowmem ();
                        if (!change)
                                return 0;
                        write_log ("NATMEM: %d, %dM > %dM = %dM\n", ++lowround, totalsize >> 20, size64 >> 20, (totalsize - change) >> 20);
                        totalsize -= change;
                }
                if ((rounds > 1 && totalsize < 0x10000000) || rounds > 20) {
                        write_log ("NATMEM: No special area could be allocated (3)!\n");
                        return 0;
                }
                natmemsize = size + z3size;

                free (memwatchtable);
                memwatchtable = 0;
                if (currprefs.gfxmem_size) {
                        if (!memwatchok) {
                                write_log ("GetWriteWatch() not supported, using guard pages, RTG performance will be slower.\n");
                                memwatchtable = xcalloc (uae_u8, currprefs.gfxmem_size / getpagesize() + 1);
                        }
                }
                if (currprefs.gfxmem_size) {
                        rtgextra = getpagesize();
                } else {
                        rtgbarrier = 0;
                        rtgextra = 0;
                }
				size = natmemsize + rtgbarrier + currprefs.gfxmem_size + rtgextra + 16 * getpagesize();
                blah = (uae_u8*)valloc (size);
				mprotect (blah, size, PROT_READ|PROT_WRITE|PROT_EXEC);

                if (blah) {
                        natmem_offset = blah;
                        break;
                }
                write_log ("NATMEM: %dM area failed to allocate, err=%d (Z3=%dM,RTG=%dM)\n",
                        natmemsize >> 20, errno, (currprefs.z3fastmem_size + currprefs.z3fastmem2_size) >> 20, currprefs.gfxmem_size >> 20);
                if (!lowmem ()) {
                        write_log ("NATMEM: No special area could be allocated (2)!\n");
                        return 0;
                }
        }
        p96mem_size = currprefs.gfxmem_size;
        if (p96mem_size) {
                free (natmem_offset);
				size = natmemsize + rtgbarrier;
                if (!(natmem_offset = valloc (size))) {
                        write_log ("VirtualAlloc() part 2 error %d. RTG disabled.\n", errno);
                        currprefs.gfxmem_size = changed_prefs.gfxmem_size = 0;
                        rtgbarrier = getpagesize();
                        rtgextra = 0;
                        goto restart;
                }
				mprotect (natmem_offset, size, PROT_READ|PROT_WRITE|PROT_EXEC);
				size = p96mem_size + rtgextra;
                p96mem_offset = (uae_u8*)valloc (/*natmem_offset + natmemsize + rtgbarrier,*/ size);
				mprotect (p96mem_offset, size, PROT_READ|PROT_WRITE|PROT_EXEC);
                if (!p96mem_offset) {
                        currprefs.gfxmem_size = changed_prefs.gfxmem_size = 0;
                        write_log ("NATMEM: failed to allocate special Picasso96 GFX RAM, err=%d\n", errno);
                }
        }
        if (!natmem_offset) {
                write_log ("NATMEM: No special area could be allocated! (1) err=%d\n", errno);
        } else {
                write_log ("NATMEM: Our special area: 0x%p-0x%p (%08x %dM)\n",
                        natmem_offset, (uae_u8*)natmem_offset + natmemsize,
                        natmemsize, natmemsize >> 20);
                if (currprefs.gfxmem_size)
                        write_log ("NATMEM: P96 special area: 0x%p-0x%p (%08x %dM)\n",
                        p96mem_offset, (uae_u8*)p96mem_offset + currprefs.gfxmem_size,
                        currprefs.gfxmem_size, currprefs.gfxmem_size >> 20);
                canbang = 1;
                natmem_offset_end = p96mem_offset + currprefs.gfxmem_size;
        }

        resetmem ();

		return canbang;
}

void mapped_free (uae_u8 *mem)
{
	shmpiece *x = shm_start;

	if (mem == filesysory) {
		while(x) {
			if (mem == x->native_address) {
				int shmid = x->id;
				shmids[shmid].key = -1;
				shmids[shmid].name[0] = '\0';
				shmids[shmid].size = 0;
				shmids[shmid].attached = 0;
				shmids[shmid].mode = 0;
				shmids[shmid].natmembase = 0;
			}
			x = x->next;
		}
		return;
	}

	while(x) {
		if(mem == x->native_address)
			my_shmdt (x->native_address);
		x = x->next;
	}
	x = shm_start;
	while(x) {
		struct shmid_ds blah;
		if (mem == x->native_address) {
			if (my_shmctl (x->id, IPC_STAT, &blah) == 0)
				my_shmctl (x->id, IPC_RMID, &blah);
		}
		x = x->next;
	}
}

#define TRUE 1
#define FALSE 0
void *my_shmat (int shmid, void *shmaddr, int shmflg)
{
	void *result = (void *)-1;
	unsigned int got = FALSE;
	int p96special = FALSE;

#ifdef NATMEM_OFFSET
	unsigned int size = shmids[shmid].size;

	if (shmids[shmid].attached)
		return shmids[shmid].attached;

	if ((uae_u8*)shmaddr < natmem_offset) {
		if(!_tcscmp (shmids[shmid].name, "chip")) {
			shmaddr=natmem_offset;
			got = TRUE;
			if (currprefs.fastmem_size == 0 || currprefs.chipmem_size < 2 * 1024 * 1024)
				size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, "kick")) {
			shmaddr=natmem_offset + 0xf80000;
			got = TRUE;
			size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, "rom_a8")) {
			shmaddr=natmem_offset + 0xa80000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "rom_e0")) {
			shmaddr=natmem_offset + 0xe00000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "rom_f0")) {
			shmaddr=natmem_offset + 0xf00000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "rtarea")) {
			shmaddr=natmem_offset + rtarea_base;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "fast")) {
			shmaddr=natmem_offset + 0x200000;
			got = TRUE;
			size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, "ramsey_low")) {
			shmaddr=natmem_offset + a3000lmem_start;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "ramsey_high")) {
			shmaddr=natmem_offset + a3000hmem_start;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "z3")) {
			shmaddr=natmem_offset + currprefs.z3fastmem_start;
			if (!currprefs.z3fastmem2_size)
				size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "z3_2")) {
			shmaddr=natmem_offset + currprefs.z3fastmem_start + currprefs.z3fastmem_size;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "gfx")) {
			got = TRUE;
			p96special = 1;
			p96ram_start = p96mem_offset - natmem_offset;
			shmaddr = natmem_offset + p96ram_start;
			size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, "bogo")) {
			shmaddr=natmem_offset+0x00C00000;
			got = TRUE;
			if (currprefs.bogomem_size <= 0x100000)
				size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, "filesys")) {
			static uae_u8 *filesysptr;
			if (filesysptr == NULL)
				filesysptr = xcalloc (uae_u8, size);
			result = filesysptr;
			shmids[shmid].attached = result;
			return result;
		}
		if(!_tcscmp (shmids[shmid].name, "custmem1")) {
			shmaddr=natmem_offset + currprefs.custom_memory_addrs[0];
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "custmem2")) {
			shmaddr=natmem_offset + currprefs.custom_memory_addrs[1];
			got = TRUE;
		}

		if(!_tcscmp (shmids[shmid].name, "hrtmem")) {
			shmaddr=natmem_offset + 0x00a10000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "arhrtmon")) {
			shmaddr=natmem_offset + 0x00800000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "xpower_e2")) {
			shmaddr=natmem_offset + 0x00e20000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "xpower_f2")) {
			shmaddr=natmem_offset + 0x00f20000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "nordic_f0")) {
			shmaddr=natmem_offset + 0x00f00000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "nordic_f4")) {
			shmaddr=natmem_offset + 0x00f40000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "nordic_f6")) {
			shmaddr=natmem_offset + 0x00f60000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp(shmids[shmid].name, "superiv_b0")) {
			shmaddr=natmem_offset + 0x00b00000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "superiv_d0")) {
			shmaddr=natmem_offset + 0x00d00000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, "superiv_e0")) {
			shmaddr=natmem_offset + 0x00e00000;
			size += BARRIER;
			got = TRUE;
		}
	}
#endif

	if (shmids[shmid].key == shmid && shmids[shmid].size) {
		shmids[shmid].mode = 0;
		shmids[shmid].natmembase = natmem_offset;
		write_log ("SHMAddr %s %p = 0x%p - 0x%p\n", shmids[shmid].name, (uae_u8*)shmaddr-natmem_offset, shmaddr, natmem_offset);
//		if (shmaddr)
//			free (shmaddr);
		result = valloc (/*shmaddr,*/ size);
		if (result == NULL) {
			result = (void*)-1;
			write_log ("VirtualAlloc %08X - %08X %x (%dk) failed %d\n",
				(uae_u8*)shmaddr - natmem_offset, (uae_u8*)shmaddr - natmem_offset + size,
				size, size >> 10, errno);
		} else {
			shmids[shmid].attached = result;
			write_log ("VirtualAlloc %08X - %08X %x (%dk) ok%s\n",
				(uae_u8*)shmaddr - natmem_offset, (uae_u8*)shmaddr - natmem_offset + size,
				size, size >> 10, p96special ? " P96" : "");
		}
	}
	return result;
}

static key_t get_next_shmkey (void)
{
	key_t result = -1;
	int i;
	for (i = 0; i < MAX_SHMID; i++) {
		if (shmids[i].key == -1) {
			shmids[i].key = i;
			result = i;
			break;
		}
	}
	return result;
}

STATIC_INLINE key_t find_shmkey (key_t key)
{
	int result = -1;
	if(shmids[key].key == key) {
		result = key;
	}
	return result;
}

int my_shmctl (int shmid, int cmd, struct shmid_ds *buf)
{
	int result = -1;

	if ((find_shmkey (shmid) != -1) && buf) {
		switch (cmd)
		{
		case IPC_STAT:
			*buf = shmids[shmid];
			result = 0;
			break;
		case IPC_RMID:
			free (shmids[shmid].attached);
			shmids[shmid].key = -1;
			shmids[shmid].name[0] = '\0';
			shmids[shmid].size = 0;
			shmids[shmid].attached = 0;
			shmids[shmid].mode = 0;
			result = 0;
			break;
		}
	}
	return result;
}

int my_shmdt (const void *shmaddr)
{
        return 0;
}

int my_shmget (key_t key, size_t size, int shmflg, const char *name)
{
	int result = -1;

//	write_log ("key %d (%d), size %d, shmflg %d, name %s\n", key, IPC_PRIVATE, size, shmflg, name);
//	if((key == IPC_PRIVATE) || ((shmflg & IPC_CREAT) && (find_shmkey (key) == -1))) {
		write_log ("shmget of size %d (%dk) for %s\n", size, size >> 10, name);
		if ((result = get_next_shmkey ()) != -1) {
			shmids[result].size = size;
			_tcscpy (shmids[result].name, name);
		} else {
			result = -1;
		}
//	}
	return result;
}

#endif //NATMEM_OFFSET

#endif //JIT
