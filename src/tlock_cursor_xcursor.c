/*
 ============================================================================
 Name        : tlock_cursor_xcursor.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - cursor - xcursor : load a cursor from an Xcursor theme.

 Usage: -cursor xcursor[:<cursor-name>]

   <cursor-name> is the Xcursor theme cursor name (default: "watch").
   The theme is chosen by the user's $XCURSOR_THEME or the root window
   RESOURCE_MANAGER property, same as any other X application.

 Examples:
   -cursor xcursor              → "watch" cursor
   -cursor xcursor:left_ptr     → arrow cursor from current theme
   -cursor xcursor:lock         → lock cursor

 Requires libXcursor (-lXcursor).
 ============================================================================
 */

#include "tlock.h"
#include <X11/Xcursor/Xcursor.h>
#include <stdlib.h>
#include <string.h>

static Cursor xcursor_cursors[64];
static int    xcursor_nr = 0;

static const char *parse_cursor_name(const char *args)
{
	if (!args)
		return "watch";
	const char *colon = strchr(args, ':');
	return colon ? colon + 1 : "watch";
}

static int tlock_cursor_xcursor_init(const char *args, struct aXInfo *xinfo)
{
	const char *name;
	int scr;

	if (!xinfo)
		return 0;

	name      = parse_cursor_name(args);
	xcursor_nr = xinfo->nr_screens;
	if (xcursor_nr > 64) xcursor_nr = 64;

	for (scr = 0; scr < xcursor_nr; scr++) {
		xcursor_cursors[scr] = XcursorLibraryLoadCursor(xinfo->display, name);
		if (!xcursor_cursors[scr]) {
			TLOCK_WARNING("xcursor: failed to load cursor '%s', falling back to default", name);
			xcursor_cursors[scr] = None;
		}
		xinfo->cursor[scr] = xcursor_cursors[scr];
	}

	return 1;
}

static int tlock_cursor_xcursor_deinit(struct aXInfo *xinfo)
{
	int scr;
	if (!xinfo)
		return 1;
	for (scr = 0; scr < xcursor_nr; scr++) {
		if (xcursor_cursors[scr])
			XFreeCursor(xinfo->display, xcursor_cursors[scr]);
		xcursor_cursors[scr] = None;
	}
	xcursor_nr = 0;
	return 1;
}

struct aCursor tlock_cursor_xcursor = {
	"xcursor",
	tlock_cursor_xcursor_init,
	tlock_cursor_xcursor_deinit
};
