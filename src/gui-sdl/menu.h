#include <stdio.h>
#include <SDL.h>
#include "SDL_ttf.h"
#include "gp2x.h"

#define iconsizex 50
#define iconsizey 60
#define bosluk 10

extern SDL_Surface *display;

void write_text(int x, int y, char* txt);
void blit_image(SDL_Surface* img, int x, int y);
void secilimi (int ix, int iy, int mx, int my, SDL_Surface* img, int hangi);

enum { menu_sel_foo, menu_sel_expansion, menu_sel_prefs, menu_sel_keymaps, menu_sel_floppy, menu_sel_reset, menu_sel_storage, menu_sel_run, menu_sel_exit, menu_sel_tweaks };

SDL_Surface* pMouse_Pointer;
SDL_Surface* pMenu_Surface;
SDL_Surface* icon_expansion;
SDL_Surface* icon_preferences;
SDL_Surface* icon_keymaps;
SDL_Surface* icon_floppy;
SDL_Surface* icon_reset;
SDL_Surface* icon_storage;
SDL_Surface* icon_run;
SDL_Surface* icon_exit;
SDL_Surface* icon_tweaks;

TTF_Font *amiga_font;
SDL_Color text_color;
SDL_Rect rect;
