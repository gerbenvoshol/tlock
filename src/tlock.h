/*
 ============================================================================
 Name        : tlock.h
 Author      : akaan
 Version     : 
 Copyright   : Cornell University - cornell.edu
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - transparent lock, Ansi-style
 Created     : Jan 25, 2014
 ============================================================================
 */

#ifndef _TLOCK_H_
#define _TLOCK_H_

#include <X11/Xlib.h>
#include <stdio.h>
#include <syslog.h>

#ifndef VERSION
#define VERSION "1.0"
#endif

/* ----------------------------------------------------------------  *
 * LOGGING
 *
 * GCLP-compliant syslog macros.
 *
 * All security-relevant events are written to syslog so they appear in
 * the system audit trail.  Passwords are NEVER logged.  Log lines always
 * include the username so that audit records are traceable to a specific
 * technician account on a shared laboratory computer.
 *
 * Levels used:
 *   LOG_NOTICE  – normal operational events (lock/unlock, module start)
 *   LOG_WARNING – recoverable failures (bad password, group mismatch)
 *   LOG_ERR     – hard errors (cannot open display, PAM failure)
 *   LOG_ALERT   – security events (too many failed attempts)
 *
 * Debug/trace macros (only compiled-in with -DVERBOSE_FLAG etc.) write to
 * stderr and never write passwords.
 * ----------------------------------------------------------------  */

/* Always write through syslog regardless of compilation flags. */
#define TLOCK_LOG(level, ...)  syslog((level), __VA_ARGS__)

/* Convenience shortcuts. */
#define TLOCK_NOTICE(...)   TLOCK_LOG(LOG_NOTICE,  __VA_ARGS__)
#define TLOCK_WARNING(...)  TLOCK_LOG(LOG_WARNING, __VA_ARGS__)
#define TLOCK_ERR(...)      TLOCK_LOG(LOG_ERR,     __VA_ARGS__)
#define TLOCK_ALERT(...)    TLOCK_LOG(LOG_ALERT,   __VA_ARGS__)

/* Legacy _SYSLOG_ kept for backward compatibility – maps to TLOCK_NOTICE. */
#undef  _SYSLOG_
#define _SYSLOG_(...) TLOCK_NOTICE(__VA_ARGS__)

#ifdef VERBOSE_FLAG
#define LOG_VERBOSE printf( "%-20s:%-25s:%03d\n", __FILE__, __FUNCTION__,__LINE__ );
#define LOG(s)      printf( "%-20s:%-25s:%03d - %s\n",__FILE__, __FUNCTION__, __LINE__, s)
#else
#define LOG_VERBOSE
#define LOG(s)
#endif

#ifdef TRACE_FLAG
#define _PRINT_      fprintf(stderr, "%-20s:%-25s: %d\n",__FILE__,__FUNCTION__,__LINE__)
#define _PRINTF_(...) fprintf(stderr, "%-20s:%-25s: %d - ",__FILE__,__FUNCTION__,__LINE__)
#else
#define _PRINT_       /* */
#define _PRINTF_(...) /* */
#endif

#ifdef DEBUG_FLAG
#define DEBUG_EVENT_LOOP_BLANK  fprintf(stderr, "%-20s:%-25s:%03d - \n",__FILE__,__FUNCTION__,__LINE__)
#define DEBUG_EVENT_LOOP(s)     fprintf(stderr, "%-20s:%-25s:%03d - %s \n",__FILE__,__FUNCTION__,__LINE__,s)
#define DEBUG_FRAME(s)          fprintf(stderr, "%-20s:%-25s:%03d - \n",__FILE__,__FUNCTION__,__LINE__)
#define DEBUG_AUTH(...)         fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_EVENT_LOOP(s)
#define DEBUG_EVENT_LOOP_BLANK
#define DEBUG_FRAME(s)     do {} while(0)
#define DEBUG_AUTH(...)
#endif

#ifdef RENDER_FLAG
#define LOG_RENDER(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG_RENDER(...)
#endif


/* ----------------------------------------------------------------  *
 * PAM AUTHENTICATION DEFINITIONS
 * ----------------------------------------------------------------  */
#ifndef PAM_SERVICE_NAME
/* "login" is portable across distributions; override at build time with
 * -DPAM_SERVICE_NAME=\"common-auth\" (Debian/Ubuntu) or
 * -DPAM_SERVICE_NAME=\"system-auth\" (RHEL/CentOS)
 */
#define PAM_SERVICE_NAME "login"
#endif
#define STRING_LIMIT 64

extern int tabpos;

struct aXInfo {
	Display* display;

	Atom pid_atom;

	int nr_screens;

	int* width_of_root;
	int* height_of_root;

	Window* root;
	Colormap* colormap;

	Window* window;
	Cursor* cursor;

	Window dialog_window;

	/* Primary monitor geometry (for dialog placement).
	 * Falls back to full root geometry if XRandR is unavailable. */
	int primary_x;
	int primary_y;
	int primary_width;
	int primary_height;
};


struct aAuth {
	const char* name;
	int (*init)(const char* args);
	int (*auth)(const char* user, const char* pass, int as_gid);
	int (*deinit)();
};

struct aCursor {
	const char* name;
	int (*init)(const char* args, struct aXInfo* xinfo);
	int (*deinit)(struct aXInfo* xinfo);
};

struct aBackground {
	const char* name;
	int (*init)(const char* args, struct aXInfo* xinfo);
	int (*deinit)(struct aXInfo* xinfo);
};


struct aOpts {
	struct aAuth* auth;
	struct aCursor* cursor;
	struct aBackground* background;
	int flash;
	int gids;
	int test;  /* test mode: display dialog without grabbing keyboard/pointer */
};

#endif /* _TLOCK_H_ */

