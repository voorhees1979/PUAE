#include "SDL.h"
#include "SDL_image.h"

#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae.h"
#include "gui.h"
#include "zfile.h"

#define VIDEO_FLAGS SDL_HWSURFACE
SDL_Surface* tmpSDLScreen = NULL;

int seciliolan = 0;
char yol[256];
char msg[50];
char msg_status[50];

extern char launchDir[256];

extern int dirz(int parametre);
extern int tweakz(int parametre);
extern int prefz(int parametre);
int soundVolume = 100;
extern int flashLED;

//
int gui_init (void) {
	if (display == NULL) {
	    	SDL_Init (SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
		display = SDL_SetVideoMode(320,240,16,VIDEO_FLAGS);
	}
	SDL_JoystickEventState(SDL_ENABLE);
	SDL_JoystickOpen(0);
	SDL_ShowCursor(SDL_DISABLE);
  	TTF_Init();

	amiga_font = TTF_OpenFont("fonts/amiga4ever_pro2.ttf",8);
	text_color.r = 0;
	text_color.g = 0;
	text_color.b = 0;

    	pMenu_Surface	= SDL_LoadBMP("images/menu.bmp");
    	pMouse_Pointer	= SDL_LoadBMP("images/mousep.bmp");
	SDL_SetColorKey(pMouse_Pointer, SDL_SRCCOLORKEY, SDL_MapRGB(pMouse_Pointer->format, 85, 170,153));

	icon_expansion	= SDL_LoadBMP("images/icon-expansion.bmp");
	icon_preferences= SDL_LoadBMP("images/icon-preferences.bmp");
	icon_keymaps	= SDL_LoadBMP("images/icon-keymaps.bmp");
	icon_floppy	= SDL_LoadBMP("images/icon-floppy.bmp");
	icon_reset	= SDL_LoadBMP("images/icon-reset.bmp");
	icon_storage	= SDL_LoadBMP("images/icon-storage.bmp");
	icon_run	= SDL_LoadBMP("images/icon-run.bmp");
	icon_exit	= SDL_LoadBMP("images/icon-exit.bmp");
	icon_tweaks	= SDL_LoadBMP("images/icon-tweaks.bmp");

	tmpSDLScreen = SDL_CreateRGBSurface(display->flags,display->w,display->h,display->format->BitsPerPixel,display->format->Rmask,display->format->Gmask,display->format->Bmask,display->format->Amask);
	return 1;
}

void gui_exit (void){
	if (0 != 1) {
    	SDL_FreeSurface(tmpSDLScreen);

    	SDL_FreeSurface(pMenu_Surface);
    	SDL_FreeSurface(pMouse_Pointer);

	SDL_FreeSurface(icon_expansion);
	SDL_FreeSurface(icon_preferences);
	SDL_FreeSurface(icon_keymaps);
	SDL_FreeSurface(icon_floppy);
	SDL_FreeSurface(icon_reset);
	SDL_FreeSurface(icon_storage);
	SDL_FreeSurface(icon_run);
	SDL_FreeSurface(icon_exit);
	SDL_FreeSurface(icon_tweaks);
	}
	SDL_Quit;
}

int gui_display(int shortcut){
	SDL_Event event;

	int menu_exitcode = -1;
	int mainloopdone = 0;
	int mouse_x = 30;
	int mouse_y = 40;
	int kup = 0;
	int kdown = 0;
	int kleft = 0;
	int kright = 0;
	int ksel = 0;
	int iconpos_x = 0;
	int iconpos_y = 0;

	getcwd(launchDir,256);

	while (!mainloopdone) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				mainloopdone = 1;
			}
			if (event.type == SDL_JOYBUTTONDOWN) {
             			switch (event.jbutton.button) {
                 			case GP2X_BUTTON_R: break;
					case GP2X_BUTTON_L: break;
					case GP2X_BUTTON_UP: kup = 1; break;
					case GP2X_BUTTON_DOWN: kdown = 1; break;
					case GP2X_BUTTON_LEFT: kleft = 1; break;
					case GP2X_BUTTON_RIGHT: kright = 1; break;
					case GP2X_BUTTON_CLICK: ksel = 1; break;
					case GP2X_BUTTON_B: ksel = 1; break;
					case GP2X_BUTTON_Y: break;
					case GP2X_BUTTON_START: mainloopdone = 1; break;
				}
			}
      			if (event.type == SDL_KEYDOWN) {
    				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:	mainloopdone = 1; break;
				 	case SDLK_UP:		kup = 1; break;
					case SDLK_DOWN:		kdown = 1; break;
					case SDLK_LEFT:		kleft = 1; break;
					case SDLK_RIGHT:	kright = 1; break;
					case SDLK_b:		ksel = 1; break;
					default: break;
				}
			}
			if (event.type == SDL_MOUSEMOTION) {
				mouse_x += event.motion.xrel;
				mouse_y += event.motion.yrel;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (seciliolan == 0) {
					if (mouse_x >= 0 && mouse_x <= 20) {
                                		if (mouse_y >= 0 && mouse_y <= 20) {
							mainloopdone = 1;
						}
					}
				} else {
					ksel = 1; break;
				}
			}
		}
		if (ksel == 1) {
			if (seciliolan == menu_sel_expansion) {
				sprintf(msg,"%s","Select KickStart ROM");
				sprintf(msg_status,"%s"," ");
				sprintf(yol,"%s/roms",launchDir);
				dirz(1);
			}
			if (seciliolan == menu_sel_floppy) {
				sprintf(msg,"%s","Select Disk Image");
				sprintf(msg_status,"%s","DF0: B  DF1: A");
				sprintf(yol,"%s/disks",launchDir);
				dirz(0);
			}
			if (seciliolan == menu_sel_prefs) {
				sprintf(msg,"%s"," ");
				sprintf(msg_status,"%s"," ");
				prefz(0);
			}
			if (seciliolan == menu_sel_reset) {
				//reset amiga
				menu_exitcode = 2;
				mainloopdone = 1;
			}
			if (seciliolan == menu_sel_keymaps) {
			}
			if (seciliolan == menu_sel_tweaks) {
				sprintf(msg,"%s","Tweaks");
				sprintf(msg_status,"%s","L/R = -/+  B: Apply");
				tweakz(0);
			}
			if (seciliolan == menu_sel_storage) {

			}
			if (seciliolan == menu_sel_run) {
				menu_exitcode = 1;
				mainloopdone = 1;
			}
			if (seciliolan == menu_sel_exit) {
				SDL_Quit();

#ifdef GP2X
				//remove mmuhack module
				system("/sbin/rmmod mmuhack");

				//menu
				chdir("/usr/gp2x");
				execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#endif
				exit(0);
			}
			ksel = 0;
		}
	// background
		SDL_BlitSurface (pMenu_Surface,NULL,tmpSDLScreen,NULL);

	// icons
        	iconpos_x = 10;
	        iconpos_y = 23;

        	secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y,icon_floppy, menu_sel_floppy);
	        blit_image (icon_floppy, iconpos_x, iconpos_y);
        
	        iconpos_x += iconsizex + bosluk;
        	secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y, icon_preferences, menu_sel_prefs);
	        blit_image (icon_preferences, iconpos_x, iconpos_y);
        
	        iconpos_x += iconsizex + bosluk;
        	secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y, icon_tweaks, menu_sel_tweaks);
	        blit_image (icon_tweaks, iconpos_x, iconpos_y);

        	iconpos_x += iconsizex + bosluk;
	        secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y, icon_keymaps, menu_sel_keymaps);
        	blit_image (icon_keymaps, iconpos_x, iconpos_y);
        
	        iconpos_x += iconsizex + bosluk;
	        secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y, icon_expansion, menu_sel_expansion);
        	blit_image (icon_expansion, iconpos_x, iconpos_y);

        	iconpos_x = 10;
	        iconpos_y = 93;

        	secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y,icon_storage, menu_sel_storage);
	        blit_image (icon_storage, iconpos_x, iconpos_y);

	        iconpos_x += iconsizex + bosluk;
	        secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y, icon_reset, menu_sel_reset);
        	blit_image (icon_reset, iconpos_x, iconpos_y);

	        iconpos_x += iconsizex + bosluk;
        	secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y, icon_run, menu_sel_run);
	        blit_image (icon_run, iconpos_x, iconpos_y);
        
        	iconpos_x += iconsizex + bosluk;
	        secilimi (iconpos_x,iconpos_y,mouse_x,mouse_y, icon_exit, menu_sel_exit);
        	blit_image (icon_exit, iconpos_x, iconpos_y);
	// texts
		write_text (26,3,"UAE2x 0.1 alpha  //GnoStiC");

	// mouse pointer ------------------------------
		if (kleft == 1) {
	                mouse_x -= (iconsizex + bosluk);
        	        kleft = 0;
		}
		if (kright == 1) {
	                mouse_x += (iconsizex + bosluk);
	                kright = 0;
		}
	        if (kup == 1) {
	                mouse_y -= (iconsizey + bosluk);
	                kup = 0;
		}
		if (kdown == 1) {
        	        kdown = 0;
	                mouse_y += (iconsizey + bosluk);
		}

        	if (mouse_x < 1) { mouse_x = 1; }
	        if (mouse_y < 1) { mouse_y = 1; }
        	if (mouse_x > 320) { mouse_x = 320; }
	        if (mouse_y > 240) { mouse_y = 240; }
		rect.x = mouse_x;
		rect.y = mouse_y;
		//rect.w = pMouse_Pointer->w;
		//rect.h = pMouse_Pointer->h;
		SDL_BlitSurface (pMouse_Pointer,NULL,tmpSDLScreen,&rect);
	// mouse pointer-end

		SDL_BlitSurface (tmpSDLScreen,NULL,display,NULL);
		SDL_Flip(display);
	} //while done

	return menu_exitcode;
}
                                                            
void write_text(int x, int y, char* txt) {
	SDL_Surface* pText_Surface = TTF_RenderText_Solid(amiga_font, txt, text_color);

	rect.x = x;
	rect.y = y;
	rect.w = pText_Surface->w;
	rect.h = pText_Surface->h;

	SDL_BlitSurface (pText_Surface,NULL,tmpSDLScreen,&rect);
	SDL_FreeSurface(pText_Surface);
}
void blit_image(SDL_Surface* img, int x, int y) {
	SDL_Rect dest;
   	dest.x = x;
   	dest.y = y;
   	SDL_BlitSurface(img, 0, tmpSDLScreen, &dest);
}

void secilimi (int ix, int iy, int mx, int my, SDL_Surface* img, int hangi) {
        int secili = 0;
        if (mx >= ix && mx <= ix + iconsizex) {
                if (my >= iy && my <= iy + iconsizey) {
                    secili = 1;
                }
        }
        if (secili == 1) {
            SDL_SetAlpha(img, SDL_SRCALPHA, 100);
            seciliolan = hangi;
        } else {
            SDL_SetAlpha(img, SDL_SRCALPHA, 255);
	}
}
//
static void sigchldhandler(int foo) {}
int gui_update (void){ return 0; }
void gui_fps (int fps, int idle){
    gui_data.fps  = fps;
    gui_data.idle = idle;
}
void gui_flicker_led (int led, int unitnum, int status){}
void gui_led (int led, int on){}
void gui_hd_led (int led){
    static int resetcounter;
    int old = gui_data.hd;

    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.hd = led;
    resetcounter = 6;
    if (old != gui_data.hd)
	gui_led (5, gui_data.hd);
}

void gui_cd_led (int led){
    static int resetcounter;

    int old = gui_data.cd;
    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.cd = led;
    resetcounter = 6;
    if (old != gui_data.cd)
	gui_led (6, gui_data.cd);
}

void gui_filename (int num, const char *name){}
void gui_handle_events (void){}
void gui_changesettings (void){}
void gui_update_gfx (void){}
void gui_lock (void){}
void gui_unlock (void){}
void gui_message (const char *format,...){   
       char msg[2048];
       va_list parms;

       va_start (parms,format);
       vsprintf ( msg, format, parms);
       va_end (parms);

       write_log (msg);
}
