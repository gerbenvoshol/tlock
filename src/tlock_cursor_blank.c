/*
 ============================================================================
 Name        : tlock_cursor_blank.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - cursor - blank : hide the mouse pointer while locked.

 The standard technique is to create a 1×1 bitmap with all bits zero and
 use it as both the cursor pixmap and mask.  The resulting cursor is fully
 transparent / invisible.
 ============================================================================
 */

#include "tlock.h"

static Cursor blank_cursors[64]; /* enough for any realistic nr_screens */
static int    blank_nr = 0;

static int tlock_cursor_blank_init(const char *args, struct aXInfo *xinfo)
{
	static const char zero_data[1] = { 0 };
	XColor black = { 0, 0, 0, 0, 0, 0 };
	int scr;

	if (!xinfo)
		return 0;

	blank_nr = xinfo->nr_screens;
	if (blank_nr > 64) blank_nr = 64;

	for (scr = 0; scr < blank_nr; scr++) {
		Pixmap pm = XCreateBitmapFromData(xinfo->display,
		                                  xinfo->root[scr],
		                                  zero_data, 1, 1);
		blank_cursors[scr] = XCreatePixmapCursor(xinfo->display,
		                                          pm, pm,
		                                          &black, &black,
		                                          0, 0);
		XFreePixmap(xinfo->display, pm);
		xinfo->cursor[scr] = blank_cursors[scr];
	}

	return 1;
}

static int tlock_cursor_blank_deinit(struct aXInfo *xinfo)
{
	int scr;
	if (!xinfo)
		return 1;
	for (scr = 0; scr < blank_nr; scr++) {
		if (blank_cursors[scr])
			XFreeCursor(xinfo->display, blank_cursors[scr]);
		blank_cursors[scr] = None;
	}
	blank_nr = 0;
	return 1;
}

struct aCursor tlock_cursor_blank = {
	"blank",
	tlock_cursor_blank_init,
	tlock_cursor_blank_deinit
};
