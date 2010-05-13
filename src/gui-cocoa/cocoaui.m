/*
 * UAE - The Un*x Amiga Emulator
 *
 * Interface to the Cocoa Mac OS X GUI
 *
 * Copyright 1996 Bernd Schmidt
 * Copyright 2004,2010 Steven J. Saunders
 */
#include <stdlib.h>
#include <stdarg.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "options.h"
#include "gui.h"
#include "inputdevice.h"
#include "disk.h"
#include "ar.h"

#include "custom.h"
#include "xwin.h"
#include "drawing.h"

#ifdef USE_SDL
#include "SDL.h"
#endif

#import <Cocoa/Cocoa.h>

/* The GTK GUI code seems to use 255 as max path length. Not sure why it 
 * doesn't use MAX_DPATH... but we will follow its example.
 */
#define COCOA_GUI_MAX_PATH 255

/* These prototypes aren't declared in the sdlgfx header for some reason */
extern void toggle_fullscreen (void);
extern int is_fullscreen (void);

/* Defined in SDLmain.m */
extern NSString *getApplicationName(void);

/* Prototypes */
int ensureNotFullscreen (void);
void restoreFullscreen (void);
void lossyASCIICopy (char *buffer, NSString *source, size_t maxLength);

/* Globals */
static BOOL wasFullscreen = NO; // used by ensureNotFullscreen() and restoreFullscreen()

/* Objective-C class for an object to respond to events */
@interface PuaeGui : NSObject
{
    NSString *applicationName;
    NSArray *diskImageTypes;
}
+ (id) sharedInstance;
- (void)createMenus;
- (void)createMenuItemInMenu:(NSMenu *)menu withTitle:(NSString *)title action:(SEL)anAction tag:(int)tag;
- (void)createMenuItemInMenu:(NSMenu *)menu withTitle:(NSString *)title action:(SEL)anAction tag:(int)tag
    keyEquivalent:(NSString *)keyEquiv keyEquivalentMask:(NSUInteger)mask;
- (BOOL)validateMenuItem:(id <NSMenuItem>)item;
- (void)insertDisk:(id)sender;
- (void)ejectDisk:(id)sender;
- (void)ejectAllDisks:(id)sender;
- (void)changePort0:(id)sender;
- (void)changePort1:(id)sender;
- (void)swapGamePorts:(id)sender;
- (void)displayOpenPanelForInsertIntoDriveNumber:(int)driveNumber;
- (void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
- (void)resetAmiga:(id)sender;
- (void)pauseAmiga:(id)sender;
#ifdef ACTION_REPLAY
- (void)actionReplayFreeze:(id)sender;
#endif
- (void)grabMouse:(id)sender;
- (void)goFullscreen:(id)sender;
- (void)toggleInhibitDisplay:(id)sender;
@end

@implementation PuaeGui

+ (id) sharedInstance
{
    static id sharedInstance = nil;

    if (sharedInstance == nil) sharedInstance = [[self alloc] init];

    return sharedInstance;
}

-(PuaeGui *) init
{
    self = [super init];

    if (self) {
        applicationName = [[NSString alloc] initWithString:getApplicationName()];
        diskImageTypes =[[NSArray alloc] initWithObjects:@"adf", @"adz",
            @"zip", @"dms", @"fdi", 
#ifdef CAPS        
            @"ipf",
#endif
            nil]; // Note: Use lowercase for these
    }

    return self;
}

-(void) dealloc
{
    [applicationName release];
    [diskImageTypes release];
    [super dealloc];
}

-(NSArray *) diskImageTypes
{
    return diskImageTypes;
}

-(NSString *)applicationName
{
    return applicationName;
}

- (void)createMenus
{
    int driveNumber;
    NSMenuItem *menuItem;
    NSString *menuTitle;

	// Create a menu for manipulating the emulated amiga
	NSMenu *vAmigaMenu = [[NSMenu alloc] initWithTitle:@"Virtual Amiga"];
	
	[self createMenuItemInMenu:vAmigaMenu withTitle:@"Cold Reset" action:@selector(resetAmiga:) tag:1];
	[self createMenuItemInMenu:vAmigaMenu withTitle:@"Warm Reset" action:@selector(resetAmiga:) tag:0];
	[self createMenuItemInMenu:vAmigaMenu withTitle:@"Pause" action:@selector(pauseAmiga:) tag:0];
	
#ifdef ACTION_REPLAY
	[self createMenuItemInMenu:vAmigaMenu
                     withTitle:@"Action Replay Freeze"
                        action:@selector(actionReplayFreeze:)
                           tag:0];
#endif

	[vAmigaMenu addItem:[NSMenuItem separatorItem]];
	
	// Add menu items for inserting into floppy drives 1 - 4
	NSMenu *insertFloppyMenu = [[NSMenu alloc] initWithTitle:@"Insert Floppy"];
	
	for (driveNumber=0; driveNumber<4; driveNumber++) {
        [self createMenuItemInMenu:insertFloppyMenu
                         withTitle:[NSString stringWithFormat:@"DF%d...",driveNumber]
                            action:@selector(insertDisk:)
                               tag:driveNumber];
    }

	menuItem = [[NSMenuItem alloc] initWithTitle:@"Insert Floppy" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:insertFloppyMenu];
	[vAmigaMenu addItem:menuItem];
	[menuItem release];
	
	[insertFloppyMenu release];
	
	// Add menu items for ejecting from floppy drives 1 - 4
	NSMenu *ejectFloppyMenu = [[NSMenu alloc] initWithTitle:@"Eject Floppy"];
	
	[self createMenuItemInMenu:ejectFloppyMenu withTitle:@"All" action:@selector(ejectAllDisks:) tag:0];
	
	[ejectFloppyMenu addItem:[NSMenuItem separatorItem]];
	
	for (driveNumber=0; driveNumber<4; driveNumber++) {
        [self createMenuItemInMenu:ejectFloppyMenu
                         withTitle:[NSString stringWithFormat:@"DF%d",driveNumber]
                            action:@selector(ejectDisk:)
                               tag:driveNumber];
    }

	menuItem = [[NSMenuItem alloc] initWithTitle:@"Eject Floppy" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:ejectFloppyMenu];
	[vAmigaMenu addItem:menuItem];
	[menuItem release];
	
	[ejectFloppyMenu release];

	menuItem = [[NSMenuItem alloc] initWithTitle:@"Virtual Amiga" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:vAmigaMenu];

	[[NSApp mainMenu] insertItem:menuItem atIndex:1];
	
	[menuItem release];
	[vAmigaMenu release];
	
    // Create a menu for changing aspects of emulator control
    NSMenu *controlMenu = [[NSMenu alloc] initWithTitle:@"Control"];

	NSMenu *portMenu = [[NSMenu alloc] initWithTitle:@"Game Port 0"];

    [self createMenuItemInMenu:portMenu withTitle:@"None" action:@selector(changePort0:) tag:JSEM_END];
    [self createMenuItemInMenu:portMenu withTitle:@"Mouse" action:@selector(changePort0:) tag:JSEM_MICE];
    [self createMenuItemInMenu:portMenu withTitle:@"Joystick" action:@selector(changePort0:) tag:JSEM_JOYS];
    [self createMenuItemInMenu:portMenu withTitle:@"Second Joystick" action:@selector(changePort0:) tag:JSEM_JOYS+1];
    [self createMenuItemInMenu:portMenu withTitle:@"Numeric Keypad 2/4/6/8 + 5" action:@selector(changePort0:) tag:JSEM_KBDLAYOUT];
    [self createMenuItemInMenu:portMenu withTitle:@"Cursor Keys + Right Ctrl/Alt" action:@selector(changePort0:) tag:JSEM_KBDLAYOUT+1];
    [self createMenuItemInMenu:portMenu withTitle:@"Keyboard T/B/F/H + Left Alt" action:@selector(changePort0:) tag:JSEM_KBDLAYOUT+2];

    menuItem = [[NSMenuItem alloc] initWithTitle:@"Game Port 0" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:portMenu];
    [controlMenu addItem:menuItem];
    [menuItem release];

	[portMenu release];
	
	portMenu = [[NSMenu alloc] initWithTitle:@"Game Port 1"];

    [self createMenuItemInMenu:portMenu withTitle:@"None" action:@selector(changePort1:) tag:JSEM_END];
    [self createMenuItemInMenu:portMenu withTitle:@"Mouse" action:@selector(changePort1:) tag:JSEM_MICE];
    [self createMenuItemInMenu:portMenu withTitle:@"Joystick" action:@selector(changePort1:) tag:JSEM_JOYS];
    [self createMenuItemInMenu:portMenu withTitle:@"Second Joystick" action:@selector(changePort1:) tag:JSEM_JOYS+1];
    [self createMenuItemInMenu:portMenu withTitle:@"Numeric Keypad 2/4/6/8 + 5" action:@selector(changePort1:) tag:JSEM_KBDLAYOUT];
    [self createMenuItemInMenu:portMenu withTitle:@"Cursor Keys + Right Ctrl/Alt" action:@selector(changePort1:) tag:JSEM_KBDLAYOUT+1];
    [self createMenuItemInMenu:portMenu withTitle:@"Keyboard T/B/F/H + Left Alt" action:@selector(changePort1:) tag:JSEM_KBDLAYOUT+2];

    menuItem = [[NSMenuItem alloc] initWithTitle:@"Game Port 1" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:portMenu];
    [controlMenu addItem:menuItem];
    [menuItem release];

	[portMenu release];

	[self createMenuItemInMenu:controlMenu withTitle:@"Swap Port 0 and 1" action:@selector(swapGamePorts:) tag:0];

	[controlMenu addItem:[NSMenuItem separatorItem]];
	
	[self createMenuItemInMenu:controlMenu withTitle:@"Grab Mouse" action:@selector(grabMouse:) tag:0 
		keyEquivalent:@"g" keyEquivalentMask:NSCommandKeyMask|NSAlternateKeyMask];
	
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Control" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:controlMenu];

    [[NSApp mainMenu] insertItem:menuItem atIndex:2];

    [controlMenu release];
    [menuItem release];

	// Create a menu for changing aspects of emulator control
    NSMenu *displayMenu = [[NSMenu alloc] initWithTitle:@"Display"];

	[self createMenuItemInMenu:displayMenu withTitle:@"Fullscreen" action:@selector(goFullscreen:) tag:0 
		keyEquivalent:@"s" keyEquivalentMask:NSCommandKeyMask|NSAlternateKeyMask];
		
	[self createMenuItemInMenu:displayMenu withTitle:@"Inhibit" action:@selector(toggleInhibitDisplay:) tag:0];
	
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Display" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:displayMenu];

    [[NSApp mainMenu] insertItem:menuItem atIndex:3];

	[displayMenu release];
	[menuItem release];
}

- (void)createMenuItemInMenu:(NSMenu *)menu withTitle:(NSString *)title action:(SEL)anAction tag:(int)tag
{
	[self createMenuItemInMenu:menu withTitle:title action:anAction tag:tag
		keyEquivalent:@"" keyEquivalentMask:NSCommandKeyMask];
}

- (void)createMenuItemInMenu:(NSMenu *)menu withTitle:(NSString *)title action:(SEL)anAction tag:(int)tag
    keyEquivalent:(NSString *)keyEquiv keyEquivalentMask:(NSUInteger)mask
{
    NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:title action:anAction keyEquivalent:keyEquiv];
	[menuItem setKeyEquivalentModifierMask:mask];
    [menuItem setTag:tag];
    [menuItem setTarget:self];
    [menu addItem:menuItem];
    [menuItem release];
}

- (BOOL)validateMenuItem:(id <NSMenuItem>)item
{
	NSMenuItem *menuItem = (NSMenuItem *)item;
	
	BOOL canSetHidden = [menuItem respondsToSelector:@selector(setHidden:)];
	
    SEL menuAction = [menuItem action];
    int tag = [menuItem tag];

    // Disabled drives can't have disks inserted or ejected
    if ((menuAction == @selector(insertDisk:)) || (menuAction == @selector(ejectDisk:))) {
		if (gui_data.drive_disabled[tag]) {
			//if (canSetHidden) [menuItem setHidden:YES];
			return NO;
		} else {
			//if (canSetHidden) [menuItem setHidden:NO];
		}
	}
        
    // Eject DFx should be disabled if there's no disk in DFx
	if (menuAction == @selector(ejectDisk:)) {
		if (disk_empty(tag)) {
			[menuItem setTitle:[NSString stringWithFormat:@"DF%d",tag]];
			return NO;
		}
		
		// There's a disk in the drive, show its name in the menu item
		NSString *diskImage = [[NSString stringWithCString:gui_data.df[tag] encoding:NSASCIIStringEncoding] lastPathComponent];
		[menuItem setTitle:[NSString stringWithFormat:@"DF%d (%@)",tag,diskImage]];
		//if (canSetHidden) [menuItem setHidden:NO];
		return YES;
	}

    // The current settings for the joystick/mouse ports should be indicated
    if (menuAction == @selector(changePort0:)) {
        if (currprefs.jports[0].id == tag) [menuItem setState:NSOnState];
        else [menuItem setState:NSOffState];

        // and joystick options should be unavailable if there are no joysticks
        if (((tag == JSEM_JOYS) || (tag == (JSEM_JOYS+1)))) {
			if ((tag - JSEM_JOYS) >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
				return NO;
		}

        // and we should not allow both ports to be set to the same setting
        if ((tag != JSEM_END) && (currprefs.jports[1].id == tag))
            return NO;

        return YES;
    }

	// Repeat the above for Port 1
    if (menuAction == @selector(changePort1:)) {
        if (currprefs.jports[1].id == tag) [menuItem setState:NSOnState];
        else [menuItem setState:NSOffState];

        if (((tag == JSEM_JOYS) || (tag == (JSEM_JOYS+1)))) {
			if ((tag - JSEM_JOYS) >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
				return NO;
		}

        if ((tag != JSEM_END) && (currprefs.jports[0].id == tag))
            return NO;

        return YES;
    }

	if (menuAction == @selector(pauseAmiga:)) {
		if (pause_emulation)
			[menuItem setTitle:@"Resume"];
		else
			[menuItem setTitle:@"Pause"];
		
		return YES;
	}
	
	if (menuAction == @selector(toggleInhibitDisplay:)) {
		if (inhibit_frame) [menuItem setState:NSOnState];
		else [menuItem setState:NSOffState];
	}

	if (menuAction == @selector(actionReplayFreeze:)) 
		return ( (hrtmon_flag == ACTION_REPLAY_IDLE) || (action_replay_flag == ACTION_REPLAY_IDLE) );
	
    return YES;
}

// Invoked when the user selects one of the 'Insert DFx:' menu items
- (void)insertDisk:(id)sender
{
    [self displayOpenPanelForInsertIntoDriveNumber:[((NSMenuItem*)sender) tag]];
}

// Invoked when the user selects one of the 'Eject DFx:' menu items
- (void)ejectDisk:(id)sender
{
    disk_eject([((NSMenuItem*)sender) tag]);
}

// Invoked when the user selects "Eject All Disks"
- (void)ejectAllDisks:(id)sender
{
	int i;
	for (i=0; i<4; i++)
		if ((!gui_data.drive_disabled[i]) && (!disk_empty(i)))
			disk_eject(i);
}

// Invoked when the user selects an option from the 'Port 0' menu
- (void)changePort0:(id)sender
{
    changed_prefs.jports[0].id = [((NSMenuItem*)sender) tag];

    if( changed_prefs.jports[0].id != currprefs.jports[0].id )
        inputdevice_config_change();
}

// Invoked when the user selects an option from the 'Port 1' menu
- (void)changePort1:(id)sender
{
    changed_prefs.jports[1].id = [((NSMenuItem*)sender) tag];

    if( changed_prefs.jports[1].id != currprefs.jports[1].id )
        inputdevice_config_change();
}

- (void)swapGamePorts:(id)sender
{
	changed_prefs.jports[0].id = currprefs.jports[1].id;
	changed_prefs.jports[1].id = currprefs.jports[0].id;
	inputdevice_config_change();
}

- (void)displayOpenPanelForInsertIntoDriveNumber:(int)driveNumber
{
    ensureNotFullscreen();

    NSOpenPanel *oPanel = [NSOpenPanel openPanel];
    [oPanel setTitle:[NSString stringWithFormat:@"%@: Insert Disk Image",applicationName]];

    // Make sure setMessage (OS X 10.3+) is available before calling it
    if ([oPanel respondsToSelector:@selector(setMessage:)])
        [oPanel setMessage:[NSString stringWithFormat:@"Select a Disk Image for DF%d:", driveNumber]];

    [oPanel setPrompt:@"Choose"];
    NSString *contextInfo = [[NSString alloc] initWithFormat:@"%d",driveNumber];

	// Recall the path of the disk image that was loaded last time 
	NSString *nsfloppypath = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastUsedDiskImagePath"];
	
	/* If the configuration includes a setting for the "floppy_path" attribute
	 * start the OpenPanel in that directory.. but only the first time.
	 */
	static int run_once = 0;
	if (!run_once) {
		run_once++;
		
		const char *floppy_path = prefs_get_attr("floppy_path");
		
		if (floppy_path != NULL) {
			char homedir[MAX_PATH];
			snprintf(homedir, MAX_PATH, "%s/", getenv("HOME"));
			
			/* The default value for floppy_path is "$HOME/". We only want to use it if the
			 * user provided an actual value though, so we don't use it if it equals "$HOME/"
			 */
			if (strncmp(floppy_path, homedir, MAX_PATH) != 0)
				nsfloppypath = [NSString stringWithCString:floppy_path encoding:NSASCIIStringEncoding];
		}
	}

    [oPanel beginSheetForDirectory:nsfloppypath file:nil
                             types:diskImageTypes
                    modalForWindow:[NSApp mainWindow]
                     modalDelegate:self
                    didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                       contextInfo:contextInfo];
}

// Called when a floppy selection panel ended
- (void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
#if 0 // This currently breaks
    restoreFullscreen();
#endif

    if (returnCode != NSOKButton) return;

    int drive = [((NSString*)contextInfo) intValue];
    [((NSString*)contextInfo) release];

    if ((drive >= 0) && (drive < 4)) {
	    NSArray *files = [sheet filenames];
	    NSString *file = [files objectAtIndex:0];
		
		lossyASCIICopy (changed_prefs.df[drive], file, COCOA_GUI_MAX_PATH);
		
		// Save the path of this disk image so that future open panels can start in the same directory
		[[NSUserDefaults standardUserDefaults] setObject:[file stringByDeletingLastPathComponent] forKey:@"LastUsedDiskImagePath"];
	}
}

- (void)resetAmiga:(id)sender
{
	uae_reset([((NSMenuItem *)sender) tag]);
}

- (void)pauseAmiga:(id)sender
{
	pausemode(-1); // Found in inputdevice.c -- toggles pause mode when arg is -1
}

#ifdef ACTION_REPLAY
- (void)actionReplayFreeze:(id)sender
{
	action_replay_freeze();
}
#endif

- (void)grabMouse:(id)sender
{
	toggle_mousegrab ();
}

- (void)goFullscreen:(id)sender
{
	toggle_fullscreen();
}

- (void)toggleInhibitDisplay:(id)sender
{
	toggle_inhibit_frame (IHF_SCROLLLOCK);
}

@end

/*
 * Revert to windowed mode if in fullscreen mode. Returns 1 if the
 * mode was initially fullscreen and was successfully changed. 0 otherwise.
 */
int ensureNotFullscreen (void)
{
    int result = 0;

    if (is_fullscreen ()) {
		toggle_fullscreen ();

		if (is_fullscreen ())
			write_log ("Cannot activate GUI in full-screen mode\n");
		else {
		  result = 1;
		  wasFullscreen = YES;
        }
        }
#ifdef USE_SDL
    // Un-hide the mouse
    SDL_ShowCursor(SDL_ENABLE);
#endif

    return result;
}

void restoreFullscreen (void)
{
#ifdef USE_SDL
    // Re-hide the mouse
    SDL_ShowCursor(SDL_DISABLE);
#endif

    if ((!is_fullscreen ()) && (wasFullscreen == YES))
        toggle_fullscreen();

    wasFullscreen = NO;
}

/* Make a null-terminated copy of the source NSString into buffer using lossy
 * ASCII conversion. (Apple deprecated the 'lossyCString' method in NSString)
 */
void lossyASCIICopy (char *buffer, NSString *source, size_t maxLength)
{
	if (source == nil) {
		buffer[0] = '\0';
		return;
	}
	
	NSData *data = [source dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
	
	if (data == nil) {
		buffer[0] = '\0';
		return;
	}
	
	[data getBytes:buffer length:maxLength];
	
	/* Ensure null termination */
	NSUInteger len = [data length];
	buffer[(len >= maxLength) ? (maxLength - 1) : len] = '\0';
}

/* This function is called from od-macosx/main.m
 * WARNING: This gets called *before* real_main(...)!
 */
void cocoa_gui_early_setup (void)
{
	[[PuaeGui sharedInstance] createMenus];
}

int gui_init (void)
{
}

int gui_open (void)
{
    return -1;
}

int gui_update (void)
{
    return 0;
}

void gui_exit (void)
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

void gui_cd_led (int unitnum, int led)
{
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

void gui_filename (int num, const char *name)
{
}

static void getline (char *p)
{
}

void gui_handle_events (void)
{
}

void gui_notify_state (int state)
{
}

void gui_display (int shortcut)
{
    int result;

    if ((shortcut >= 0) && (shortcut < 4)) {
        [[PuaeGui sharedInstance] displayOpenPanelForInsertIntoDriveNumber:shortcut];
    }
}

void gui_message (const char *format,...)
{
    char msg[2048];
    va_list parms;

    ensureNotFullscreen ();

    va_start (parms,format);
    vsprintf (msg, format, parms);
    va_end (parms);

    NSRunAlertPanel(nil, [NSString stringWithCString:msg encoding:NSASCIIStringEncoding], nil, nil, nil);

    write_log ("%s", msg);

    restoreFullscreen ();
}
void gui_disk_image_change (int unitnum, const TCHAR *name) {}
void gui_lock (void) {}
void gui_unlock (void) {}

