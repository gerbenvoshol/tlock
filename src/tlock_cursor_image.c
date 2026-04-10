/*
 ============================================================================
 Name        : tlock_cursor_image.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - cursor - image : use a PNG file as the cursor.

 Usage: -cursor image:<path>[:<hotspot-x>,<hotspot-y>]

   <path>       – absolute path to a PNG (RGBA) file; recommended size ≤64×64.
   <hotspot-x>  – X hotspot offset within the image (default: 0).
   <hotspot-y>  – Y hotspot offset within the image (default: 0).

 Requires libXcursor (-lXcursor) and libpng (-lpng).
 ============================================================================
 */

#include "tlock.h"
#include <X11/Xcursor/Xcursor.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <png.h>

static Cursor img_cursors[64];
static int    img_cursor_nr = 0;

/* ------------------------------------------------------------------ */

/* Parse path and optional hotspot from args string:
 *   "image:/path/to/cursor.png:4,4"
 */
static void parse_cursor_args(const char *args,
                               const char **path_out,
                               int *hx_out, int *hy_out)
{
	static char path_buf[4096];
	*path_out = NULL;
	*hx_out   = 0;
	*hy_out   = 0;

	if (!args)
		return;

	const char *colon1 = strchr(args, ':');
	if (!colon1)
		return;
	colon1++;

	const char *colon2 = strchr(colon1, ':');
	size_t plen = colon2 ? (size_t)(colon2 - colon1) : strlen(colon1);
	if (plen >= sizeof(path_buf))
		plen = sizeof(path_buf) - 1;
	memcpy(path_buf, colon1, plen);
	path_buf[plen] = '\0';
	*path_out = path_buf;

	if (colon2) {
		sscanf(colon2 + 1, "%d,%d", hx_out, hy_out);
	}
}

/* Load RGBA data from a PNG into an XcursorImage. */
static XcursorImage *load_png_as_xcursor(const char *path, int hx, int hy)
{
	FILE       *fp;
	png_structp png_ptr;
	png_infop   info_ptr;
	unsigned int y;

	fp = fopen(path, "rb");
	if (!fp) {
		TLOCK_ERR("cursor_image: cannot open '%s'", path);
		return NULL;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) { fclose(fp); return NULL; }

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return NULL;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return NULL;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	int width  = (int)png_get_image_width(png_ptr,  info_ptr);
	int height = (int)png_get_image_height(png_ptr, info_ptr);
	int bit_depth  = png_get_bit_depth(png_ptr,  info_ptr);
	int color_type = png_get_color_type(png_ptr, info_ptr);

	/* Normalise to 8-bit RGBA. */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	png_read_update_info(png_ptr, info_ptr);

	XcursorImage *ci = XcursorImageCreate(width, height);
	if (!ci) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return NULL;
	}
	ci->xhot = (XcursorDim)hx;
	ci->yhot = (XcursorDim)hy;

	/* Read rows into a temporary byte buffer, then convert to
	 * XcursorPixel (ARGB, pre-multiplied). */
	unsigned char *row = (unsigned char *)malloc((size_t)(width * 4));
	if (!row) {
		XcursorImageDestroy(ci);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return NULL;
	}

	for (y = 0; y < (unsigned)height; y++) {
		png_read_row(png_ptr, row, NULL);
		unsigned int x;
		for (x = 0; x < (unsigned)width; x++) {
			unsigned char r = row[x * 4 + 0];
			unsigned char g = row[x * 4 + 1];
			unsigned char b = row[x * 4 + 2];
			unsigned char a = row[x * 4 + 3];
			/* XcursorPixel = 0xAARRGGBB */
			ci->pixels[y * (unsigned)width + x] =
				((XcursorPixel)a << 24) |
				((XcursorPixel)r << 16) |
				((XcursorPixel)g <<  8) |
				 (XcursorPixel)b;
		}
	}

	free(row);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return ci;
}

/* ------------------------------------------------------------------ */

static int tlock_cursor_image_init(const char *args, struct aXInfo *xinfo)
{
	const char    *path = NULL;
	int            hx = 0, hy = 0;
	XcursorImage  *ci  = NULL;
	int scr;

	if (!xinfo)
		return 0;

	parse_cursor_args(args, &path, &hx, &hy);
	if (!path || !*path) {
		TLOCK_ERR("cursor_image: no path supplied – usage: -cursor image:<path>[:<hx>,<hy>]");
		return 0;
	}

	ci = load_png_as_xcursor(path, hx, hy);
	if (!ci) {
		TLOCK_ERR("cursor_image: failed to load cursor image '%s'", path);
		return 0;
	}

	img_cursor_nr = xinfo->nr_screens;
	if (img_cursor_nr > 64) img_cursor_nr = 64;

	for (scr = 0; scr < img_cursor_nr; scr++) {
		img_cursors[scr] = XcursorImageLoadCursor(xinfo->display, ci);
		xinfo->cursor[scr] = img_cursors[scr];
	}

	XcursorImageDestroy(ci);
	return 1;
}

static int tlock_cursor_image_deinit(struct aXInfo *xinfo)
{
	int scr;
	if (!xinfo)
		return 1;
	for (scr = 0; scr < img_cursor_nr; scr++) {
		if (img_cursors[scr])
			XFreeCursor(xinfo->display, img_cursors[scr]);
		img_cursors[scr] = None;
	}
	img_cursor_nr = 0;
	return 1;
}

struct aCursor tlock_cursor_image = {
	"image",
	tlock_cursor_image_init,
	tlock_cursor_image_deinit
};
