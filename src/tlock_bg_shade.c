/*
 ============================================================================
 Name        : tlock_bg_shade.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - bg - shade : dim the screen content by drawing a
               semi-transparent overlay using the X RENDER extension.

 Usage: -bg shade[:<opacity>]

   <opacity> is a float in [0.0, 1.0] (default: 0.5).
   0.0 = fully transparent (invisible), 1.0 = fully opaque black (= blank).

 If the RENDER extension is unavailable the module falls back to the 'blank'
 behaviour (solid black).
 ============================================================================
 */

#include "tlock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <X11/extensions/Xrender.h>

static Window  *shade_windows = NULL;
static Picture *shade_pics    = NULL;
static int      shade_nr      = 0;

/* Parse optional opacity value after colon. */
static double parse_opacity(const char *args)
{
	if (!args)
		return 0.5;
	const char *colon = strchr(args, ':');
	if (!colon)
		return 0.5;
	double v = atof(colon + 1);
	if (v < 0.0) v = 0.0;
	if (v > 1.0) v = 1.0;
	return v;
}

static int tlock_bg_shade_init(const char *args, struct aXInfo *xinfo)
{
	double           opacity;
	int              rr_event, rr_error;
	XRenderColor     rc;
	XSetWindowAttributes xswa;
	int scr;

	if (!xinfo)
		return 0;

	/* Check RENDER extension. */
	if (!XRenderQueryExtension(xinfo->display, &rr_event, &rr_error)) {
		TLOCK_WARNING("bg_shade: X RENDER extension not available; falling back to opaque black");
		opacity = 1.0;
	} else {
		opacity = parse_opacity(args);
	}

	shade_nr      = xinfo->nr_screens;
	shade_windows = (Window *)calloc((size_t)shade_nr, sizeof(Window));
	shade_pics    = (Picture *)calloc((size_t)shade_nr, sizeof(Picture));
	if (!shade_windows || !shade_pics)
		return 0;

	/* Pre-multiply the ARGB value. */
	unsigned short alpha = (unsigned short)(opacity * 0xFFFF);
	rc.alpha  = alpha;
	rc.red    = 0;
	rc.green  = 0;
	rc.blue   = 0;

	xswa.override_redirect = True;
	xswa.background_pixel  = BlackPixel(xinfo->display, 0);

	for (scr = 0; scr < shade_nr; scr++) {
		XVisualInfo    vi_tmpl;
		XVisualInfo   *vi_list;
		Visual        *visual  = NULL;
		int            vi_cnt  = 0;
		int            depth   = 32;

		/* Try to find a 32-bit ARGB visual for proper alpha blending. */
		vi_tmpl.screen = scr;
		vi_tmpl.depth  = 32;
		vi_tmpl.class  = TrueColor;
		vi_list = XGetVisualInfo(xinfo->display,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
			&vi_tmpl, &vi_cnt);

		if (vi_list && vi_cnt > 0) {
			visual = vi_list[0].visual;
			XFree(vi_list);
		}

		if (!visual) {
			/* Fall back to root visual with 24-bit depth. */
			visual = DefaultVisual(xinfo->display, scr);
			depth  = DefaultDepth(xinfo->display, scr);
		}

		Colormap cmap = XCreateColormap(xinfo->display,
			xinfo->root[scr], visual, AllocNone);

		xswa.colormap          = cmap;
		xswa.border_pixel      = 0;
		xswa.background_pixel  = 0;

		shade_windows[scr] = XCreateWindow(
			xinfo->display, xinfo->root[scr],
			0, 0,
			(unsigned)xinfo->width_of_root[scr],
			(unsigned)xinfo->height_of_root[scr],
			0, depth, InputOutput, visual,
			CWOverrideRedirect | CWColormap |
			CWBackPixel | CWBorderPixel,
			&xswa);

		if (!shade_windows[scr]) {
			XFreeColormap(xinfo->display, cmap);
			continue;
		}

		xinfo->window[scr] = shade_windows[scr];

		/* Create an RENDER picture on the window and fill it. */
		XRenderPictFormat *fmt = XRenderFindVisualFormat(xinfo->display, visual);
		if (fmt) {
			shade_pics[scr] = XRenderCreatePicture(
				xinfo->display, shade_windows[scr], fmt, 0, NULL);
			XRenderFillRectangle(xinfo->display, PictOpSrc,
				shade_pics[scr], &rc,
				0, 0,
				(unsigned)xinfo->width_of_root[scr],
				(unsigned)xinfo->height_of_root[scr]);
		}
	}

	return 1;
}

static int tlock_bg_shade_deinit(struct aXInfo *xinfo)
{
	int scr;
	if (!xinfo)
		return 0;
	for (scr = 0; scr < shade_nr; scr++) {
		if (shade_pics && shade_pics[scr])
			XRenderFreePicture(xinfo->display, shade_pics[scr]);
		if (shade_windows && shade_windows[scr])
			XDestroyWindow(xinfo->display, shade_windows[scr]);
	}
	free(shade_windows);
	free(shade_pics);
	shade_windows = NULL;
	shade_pics    = NULL;
	shade_nr      = 0;
	return 1;
}

struct aBackground tlock_bg_shade = {
	"shade",
	tlock_bg_shade_init,
	tlock_bg_shade_deinit
};
