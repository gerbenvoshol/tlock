/*
 ============================================================================
 Name        : tlock_cursor_glyph.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - cursor - glyph : use an X11 cursor-font glyph.

 Usage: -cursor glyph[:<glyph-index>]

   <glyph-index> is a numeric index into the X11 cursor font (cursorfont.h).
   Default: 68 (XC_lock).

 Common glyph indices:
   XC_arrow=2  XC_cross=34  XC_lock=68  XC_watch=150  XC_X_cursor=0
 ============================================================================
 */

#include "tlock.h"
#include <X11/cursorfont.h>
#include <stdlib.h>
#include <string.h>

static Cursor glyph_cursors[64];
static int    glyph_nr = 0;

static unsigned int parse_glyph(const char *args)
{
	if (!args)
		return XC_watch;
	const char *colon = strchr(args, ':');
	if (!colon)
		return XC_watch;
	int v = atoi(colon + 1);
	return (v >= 0) ? (unsigned int)v : XC_watch;
}

static int tlock_cursor_glyph_init(const char *args, struct aXInfo *xinfo)
{
	unsigned int glyph;
	int scr;

	if (!xinfo)
		return 0;

	glyph    = parse_glyph(args);
	glyph_nr = xinfo->nr_screens;
	if (glyph_nr > 64) glyph_nr = 64;

	for (scr = 0; scr < glyph_nr; scr++) {
		glyph_cursors[scr] = XCreateFontCursor(xinfo->display, glyph);
		xinfo->cursor[scr] = glyph_cursors[scr];
	}

	return 1;
}

static int tlock_cursor_glyph_deinit(struct aXInfo *xinfo)
{
	int scr;
	if (!xinfo)
		return 1;
	for (scr = 0; scr < glyph_nr; scr++) {
		if (glyph_cursors[scr])
			XFreeCursor(xinfo->display, glyph_cursors[scr]);
		glyph_cursors[scr] = None;
	}
	glyph_nr = 0;
	return 1;
}

struct aCursor tlock_cursor_glyph = {
	"glyph",
	tlock_cursor_glyph_init,
	tlock_cursor_glyph_deinit
};
