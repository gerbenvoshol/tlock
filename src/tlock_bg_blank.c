/*
 ============================================================================
 Name        : tlock_bg_blank.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - bg - blank : fill all screens with a solid colour.

 Usage: -bg blank[:#RRGGBB]   (default colour: black)

 Examples:
   -bg blank          → black fill
   -bg blank:#1a1a2e  → dark navy fill
   -bg blank:gray20   → X11 named colour
 ============================================================================
 */

#include "tlock.h"
#include <stdlib.h>
#include <string.h>

static Window *bg_blank_windows = NULL;
static int     bg_blank_nr      = 0;

/* Parse the optional colour argument after the colon. */
static const char *parse_color(const char *args)
{
	if (!args)
		return "black";
	const char *colon = strchr(args, ':');
	return colon ? colon + 1 : "black";
}

static int tlock_bg_blank_init(const char *args, struct aXInfo *xinfo)
{
	const char        *color_name;
	XColor             xcolor, exact;
	XSetWindowAttributes xswa;
	int scr;

	if (!xinfo)
		return 0;

	color_name = parse_color(args);

	bg_blank_nr      = xinfo->nr_screens;
	bg_blank_windows = (Window *)calloc((size_t)bg_blank_nr, sizeof(Window));
	if (!bg_blank_windows)
		return 0;

	xswa.override_redirect = True;

	for (scr = 0; scr < bg_blank_nr; scr++) {
		if (!XAllocNamedColor(xinfo->display, xinfo->colormap[scr],
		                      color_name, &xcolor, &exact)) {
			/* Fall back to black if the colour name is invalid. */
			TLOCK_WARNING("bg_blank: unknown colour '%s', falling back to black",
				color_name);
			xcolor.pixel = BlackPixel(xinfo->display, scr);
		}

		xswa.background_pixel = xcolor.pixel;

		bg_blank_windows[scr] = XCreateWindow(
			xinfo->display, xinfo->root[scr],
			0, 0,
			(unsigned)xinfo->width_of_root[scr],
			(unsigned)xinfo->height_of_root[scr],
			0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWOverrideRedirect | CWBackPixel,
			&xswa);

		if (bg_blank_windows[scr])
			xinfo->window[scr] = bg_blank_windows[scr];
	}

	return 1;
}

static int tlock_bg_blank_deinit(struct aXInfo *xinfo)
{
	int scr;
	if (!xinfo || !bg_blank_windows)
		return 0;
	for (scr = 0; scr < bg_blank_nr; scr++) {
		if (bg_blank_windows[scr])
			XDestroyWindow(xinfo->display, bg_blank_windows[scr]);
	}
	free(bg_blank_windows);
	bg_blank_windows = NULL;
	bg_blank_nr      = 0;
	return 1;
}

struct aBackground tlock_bg_blank = {
	"blank",
	tlock_bg_blank_init,
	tlock_bg_blank_deinit
};
