 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Support for Linux/USS sound
  *
  * Copyright 1997 Bernd Schmidt
  */

#define SOUNDSTUFF 1

extern int sound_fd;
extern uae_u16 paula_sndbuffer[];
extern uae_u16 *paula_sndbufpt;
extern int paula_sndbufsize;

struct sound_data
{
    int waiting_for_buffer;
    int devicetype;
    int obtainedfreq;
    int paused;
    int mute;
    int channels;
    int freq;
    int samplesize;
    int paula_sndbufsize;
    void *data;
};

STATIC_INLINE void check_sound_buffers (void)
{
/*
    if (currprefs.sound_stereo == SND_4CH_CLONEDSTEREO) {
	((uae_u16*)paula_sndbufpt)[0] = ((uae_u16*)paula_sndbufpt)[-2];
	((uae_u16*)paula_sndbufpt)[1] = ((uae_u16*)paula_sndbufpt)[-1];
	paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 2 * 2);
    } else if (currprefs.sound_stereo == SND_6CH_CLONEDSTEREO) {
	uae_s16 *p = ((uae_s16*)paula_sndbufpt);
	uae_s32 sum;
	p[2] = p[-2];
	p[3] = p[-1];
	sum = (uae_s32)(p[-2]) + (uae_s32)(p[-1]) + (uae_s32)(p[2]) + (uae_s32)(p[3]);
	p[0] = sum >> 3;
	p[1] = sum >> 3;
	paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 4 * 2);
    }
    if ((uae_u8*)paula_sndbufpt - (uae_u8*)paula_sndbuffer >= paula_sndbufsize) {
	finish_sound_buffer ();
	paula_sndbufpt = paula_sndbuffer;
    }
*/
    int size = (char *)paula_sndbufpt - (char *)paula_sndbuffer;
    write (sound_fd, paula_sndbuffer, size);
    paula_sndbufpt = paula_sndbuffer;
}

STATIC_INLINE void clear_sound_buffers (void)
{
    memset (paula_sndbuffer, 0, paula_sndbufsize);
    paula_sndbufpt = paula_sndbuffer;
}

STATIC_INLINE void set_sound_buffers (void)
{ 
}

#define AUDIO_NAME "oss"

#define PUT_SOUND_WORD(b) do { *(uae_u16 *)paula_sndbufpt = b; paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 2); } while (0)
#define PUT_SOUND_WORD_LEFT(b) do { if (currprefs.sound_filter) b = filter (b, &sound_filter_state[0]); PUT_SOUND_WORD(b); } while (0)
#define PUT_SOUND_WORD_RIGHT(b) do { if (currprefs.sound_filter) b = filter (b, &sound_filter_state[1]); PUT_SOUND_WORD(b); } while (0)
#define PUT_SOUND_WORD_LEFT2(b) do { if (currprefs.sound_filter) b = filter (b, &sound_filter_state[2]); PUT_SOUND_WORD(b); } while (0)
#define PUT_SOUND_WORD_RIGHT2(b) do { if (currprefs.sound_filter) b = filter (b, &sound_filter_state[3]); PUT_SOUND_WORD(b); } while (0)

#define PUT_SOUND_WORD_MONO(b) PUT_SOUND_WORD_LEFT(b)
#define SOUND16_BASE_VAL 0
#define SOUND8_BASE_VAL 128

#define DEFAULT_SOUND_MAXB 16384
#define DEFAULT_SOUND_MINB 16384
#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100
#define HAVE_STEREO_SUPPORT

#define FILTER_SOUND_OFF 0
#define FILTER_SOUND_EMUL 1
#define FILTER_SOUND_ON 2

#define FILTER_SOUND_TYPE_A500 0
#define FILTER_SOUND_TYPE_A1200 1
