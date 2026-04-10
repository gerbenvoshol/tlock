/*
 ============================================================================
 Name        : tlock_bg_image.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - bg - image : tile/scale a PNG image as background.

 Usage: -bg image:<path>[:<mode>]

   <path>  – absolute path to a PNG file.
   <mode>  – one of: scale (default), tile, center

 Requires libpng (build with -lpng).
 ============================================================================
 */

#include "tlock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <png.h>

static Window *img_windows = NULL;
static int     img_nr      = 0;

/* ------------------------------------------------------------------ */

typedef struct {
	unsigned char *data;
	int            width;
	int            height;
} RawImage;

/* Load a PNG file into an RGBA byte array. */
static int load_png(const char *path, RawImage *out)
{
	FILE      *fp;
	png_structp png_ptr;
	png_infop   info_ptr;
	int         bit_depth, color_type;
	unsigned int y;
	png_bytep  *row_pointers;

	fp = fopen(path, "rb");
	if (!fp) {
		TLOCK_ERR("bg_image: cannot open '%s'", path);
		return 0;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) { fclose(fp); return 0; }

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return 0;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return 0;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	out->width  = (int)png_get_image_width(png_ptr,  info_ptr);
	out->height = (int)png_get_image_height(png_ptr, info_ptr);
	bit_depth   = png_get_bit_depth(png_ptr,  info_ptr);
	color_type  = png_get_color_type(png_ptr, info_ptr);

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

	out->data = (unsigned char *)malloc((size_t)(out->width * out->height * 4));
	if (!out->data) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return 0;
	}

	row_pointers = (png_bytep *)malloc((size_t)out->height * sizeof(png_bytep));
	for (y = 0; y < (unsigned)out->height; y++)
		row_pointers[y] = out->data + y * out->width * 4;

	png_read_image(png_ptr, row_pointers);

	free(row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return 1;
}

/* Convert RGBA byte array to an XImage using the display's visual. */
static XImage *rgba_to_ximage(Display *dpy, Visual *vis, int depth,
                               const RawImage *img)
{
	int    stride = img->width * 4;
	char  *xdata  = (char *)malloc((size_t)(img->height * stride));
	if (!xdata)
		return NULL;

	/* Re-order channels to match X11 BGRA pixel layout. */
	int x, y;
	for (y = 0; y < img->height; y++) {
		for (x = 0; x < img->width; x++) {
			const unsigned char *src = img->data + (y * img->width + x) * 4;
			unsigned char       *dst = (unsigned char *)xdata + y * stride + x * 4;
			dst[0] = src[2]; /* B */
			dst[1] = src[1]; /* G */
			dst[2] = src[0]; /* R */
			dst[3] = src[3]; /* A */
		}
	}

	return XCreateImage(dpy, vis, (unsigned)depth, ZPixmap, 0,
	                    xdata, (unsigned)img->width, (unsigned)img->height,
	                    32, stride);
}

/* Nearest-neighbour scale of src into dst dimensions. */
static void scale_image(const RawImage *src, RawImage *dst, int dw, int dh)
{
	int x, y;
	dst->width  = dw;
	dst->height = dh;
	dst->data   = (unsigned char *)malloc((size_t)(dw * dh * 4));
	if (!dst->data)
		return;
	for (y = 0; y < dh; y++) {
		for (x = 0; x < dw; x++) {
			int sx = x * src->width  / dw;
			int sy = y * src->height / dh;
			memcpy(dst->data + (y * dw + x) * 4,
			       src->data + (sy * src->width + sx) * 4,
			       4);
		}
	}
}

/* ------------------------------------------------------------------ */

typedef enum { IMG_SCALE, IMG_TILE, IMG_CENTER } ImgMode;

static void parse_args(const char *args, const char **path_out, ImgMode *mode_out)
{
	static char path_buf[4096];
	*path_out  = NULL;
	*mode_out  = IMG_SCALE;

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
		colon2++;
		if      (strcmp(colon2, "tile")   == 0) *mode_out = IMG_TILE;
		else if (strcmp(colon2, "center") == 0) *mode_out = IMG_CENTER;
		else                                    *mode_out = IMG_SCALE;
	}
}

static int tlock_bg_image_init(const char *args, struct aXInfo *xinfo)
{
	const char *path = NULL;
	ImgMode     mode = IMG_SCALE;
	RawImage    src  = {NULL, 0, 0};
	int scr;

	if (!xinfo)
		return 0;

	parse_args(args, &path, &mode);
	if (!path || !*path) {
		TLOCK_ERR("bg_image: no image path supplied – usage: -bg image:<path>[:<scale|tile|center>]");
		return 0;
	}

	if (!load_png(path, &src)) {
		TLOCK_ERR("bg_image: failed to load PNG '%s'", path);
		return 0;
	}

	img_nr      = xinfo->nr_screens;
	img_windows = (Window *)calloc((size_t)img_nr, sizeof(Window));
	if (!img_windows) {
		free(src.data);
		return 0;
	}

	XSetWindowAttributes xswa;
	xswa.override_redirect = True;
	xswa.background_pixel  = BlackPixel(xinfo->display, 0);

	for (scr = 0; scr < img_nr; scr++) {
		int      sw = xinfo->width_of_root[scr];
		int      sh = xinfo->height_of_root[scr];
		int      depth  = DefaultDepth(xinfo->display, scr);
		Visual  *visual = DefaultVisual(xinfo->display, scr);

		img_windows[scr] = XCreateWindow(
			xinfo->display, xinfo->root[scr],
			0, 0, (unsigned)sw, (unsigned)sh, 0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWOverrideRedirect | CWBackPixel,
			&xswa);
		if (!img_windows[scr])
			continue;

		xinfo->window[scr] = img_windows[scr];

		/* Build pixmap for this screen. */
		Pixmap pm  = XCreatePixmap(xinfo->display, img_windows[scr],
		                           (unsigned)sw, (unsigned)sh, (unsigned)depth);
		GC gc = XCreateGC(xinfo->display, pm, 0, NULL);

		if (mode == IMG_SCALE) {
			RawImage scaled = {NULL, 0, 0};
			scale_image(&src, &scaled, sw, sh);
			if (scaled.data) {
				XImage *xi = rgba_to_ximage(xinfo->display, visual, depth, &scaled);
				if (xi) {
					XPutImage(xinfo->display, pm, gc, xi, 0, 0, 0, 0,
					          (unsigned)sw, (unsigned)sh);
					xi->data = NULL; /* free separately */
					XDestroyImage(xi);
				}
				free(scaled.data);
			}
		} else if (mode == IMG_TILE) {
			XImage *xi = rgba_to_ximage(xinfo->display, visual, depth, &src);
			if (xi) {
				int tx, ty;
				for (ty = 0; ty < sh; ty += src.height)
					for (tx = 0; tx < sw; tx += src.width)
						XPutImage(xinfo->display, pm, gc, xi,
						          0, 0, tx, ty,
						          (unsigned)src.width,
						          (unsigned)src.height);
				xi->data = NULL;
				XDestroyImage(xi);
			}
		} else { /* IMG_CENTER */
			XFillRectangle(xinfo->display, pm, gc, 0, 0,
			               (unsigned)sw, (unsigned)sh);
			XImage *xi = rgba_to_ximage(xinfo->display, visual, depth, &src);
			if (xi) {
				int ox = (sw - src.width)  / 2;
				int oy = (sh - src.height) / 2;
				int cx = ox < 0 ? -ox : 0;
				int cy = oy < 0 ? -oy : 0;
				int cw = src.width  - 2 * cx;
				int ch = src.height - 2 * cy;
				if (cw > 0 && ch > 0)
					XPutImage(xinfo->display, pm, gc, xi,
					          cx, cy,
					          ox < 0 ? 0 : ox,
					          oy < 0 ? 0 : oy,
					          (unsigned)cw, (unsigned)ch);
				xi->data = NULL;
				XDestroyImage(xi);
			}
		}

		XFreeGC(xinfo->display, gc);
		XSetWindowBackgroundPixmap(xinfo->display, img_windows[scr], pm);
		XFreePixmap(xinfo->display, pm);
	}

	free(src.data);
	return 1;
}

static int tlock_bg_image_deinit(struct aXInfo *xinfo)
{
	int scr;
	if (!xinfo || !img_windows)
		return 0;
	for (scr = 0; scr < img_nr; scr++) {
		if (img_windows[scr])
			XDestroyWindow(xinfo->display, img_windows[scr]);
	}
	free(img_windows);
	img_windows = NULL;
	img_nr      = 0;
	return 1;
}

struct aBackground tlock_bg_image = {
	"image",
	tlock_bg_image_init,
	tlock_bg_image_deinit
};
