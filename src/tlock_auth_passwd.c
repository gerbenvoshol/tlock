/*
 ============================================================================
 Name        : tlock_auth_passwd.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - authenticate using the system shadow password database
               and an optional group membership check.

 Usage: -auth passwd[,group1[,group2[,...]]]

   Without any group arguments every user whose password matches is allowed
   to unlock the screen.  When one or more group names (or numeric GIDs when
   -gids is used) are provided the user must also be a member of at least one
   of those groups.

   This module requires read access to /etc/shadow.  Either run tlock as root
   (not recommended) or install it setuid-shadow / setgid-shadow, e.g.:

     sudo chown root:shadow /usr/local/bin/tlock
     sudo chmod 2755 /usr/local/bin/tlock

 ============================================================================
 */

#include "tlock.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <sys/types.h>

#define PASSWD_MAX_GROUPS 50

/* Groups configured at init time (up to 10). */
static const char *passwd_group_entry[10];
static int         passwd_initialized = 0;

/* ------------------------------------------------------------------ */

static int tlock_auth_passwd_init(const char *args)
{
	int ic = 0;
	char *tmp;

	if (passwd_initialized) {
		TLOCK_WARNING("passwd: already initialized");
		return 0;
	}

	memset(passwd_group_entry, 0, sizeof(passwd_group_entry));

	if (args && *args) {
		tmp = strdup(args);
		strtok(tmp, ","); /* skip module name "passwd" */
		for (ic = 0; ic < 10; ic++) {
			passwd_group_entry[ic] = strtok(NULL, ",");
			if (!passwd_group_entry[ic])
				break;
			/* Make a permanent copy because strtok modifies the buffer. */
			passwd_group_entry[ic] = strdup(passwd_group_entry[ic]);
			TLOCK_NOTICE("passwd: allowed group[%d]=%s", ic, passwd_group_entry[ic]);
		}
		free(tmp);
	}

	passwd_initialized = 1;
	return 1;
}

static int tlock_auth_passwd_deinit(void)
{
	int i;
	for (i = 0; i < 10 && passwd_group_entry[i]; i++) {
		free((void *)passwd_group_entry[i]);
		passwd_group_entry[i] = NULL;
	}
	passwd_initialized = 0;
	return 1;
}

/*
 * Authenticate user against /etc/shadow using crypt(3), then verify group
 * membership if any groups were configured.
 *
 * Returns 1 on success (authenticated + authorised), 0 otherwise.
 * Passwords are never written to any log.
 */
static int tlock_auth_passwd_auth(const char *username, const char *pass, int as_gid)
{
	struct spwd   *sp;
	struct passwd *pw;
	char          *crypted;
	gid_t          gids[PASSWD_MAX_GROUPS + 1];
	int            ngroups, i, j;
	struct group  *gr;

	if (!username || !pass) {
		TLOCK_WARNING("passwd: UNLOCK_FAILED: null username or password");
		return 0;
	}

	/* Look up shadow entry. */
	errno = 0;
	sp = getspnam(username);
	if (!sp) {
		/* Either the user does not exist or we cannot read /etc/shadow. */
		if (errno == EACCES)
			TLOCK_ERR("passwd: cannot read /etc/shadow – is tlock setgid shadow?");
		else
			TLOCK_WARNING("passwd: UNLOCK_FAILED: unknown user '%s'", username);
		return 0;
	}

	/* Verify the password against the stored hash. */
	crypted = crypt(pass, sp->sp_pwdp);
	if (!crypted || strcmp(crypted, sp->sp_pwdp) != 0) {
		TLOCK_WARNING("passwd: UNLOCK_FAILED: bad password for user '%s'", username);
		return 0;
	}

	/* Password is correct.  If no groups were configured, allow access. */
	if (!passwd_group_entry[0]) {
		TLOCK_NOTICE("passwd: UNLOCK_SUCCESS: user='%s' (no group restriction)", username);
		return 1;
	}

	/* Verify group membership. */
	pw = getpwnam(username);
	if (!pw) {
		TLOCK_WARNING("passwd: UNLOCK_FAILED: getpwnam failed for '%s'", username);
		return 0;
	}

	ngroups = PASSWD_MAX_GROUPS;
	getgrouplist(username, pw->pw_gid, gids, &ngroups);

	for (i = 0; i < ngroups; i++) {
		for (j = 0; j < 10 && passwd_group_entry[j]; j++) {
			if (as_gid) {
				gid_t wanted = (gid_t)atoi(passwd_group_entry[j]);
				if (gids[i] == wanted) {
					TLOCK_NOTICE("passwd: UNLOCK_SUCCESS: user='%s' gid=%u",
						username, (unsigned)gids[i]);
					return 1;
				}
			} else {
				gr = getgrgid(gids[i]);
				if (gr && strcmp(gr->gr_name, passwd_group_entry[j]) == 0) {
					TLOCK_NOTICE("passwd: UNLOCK_SUCCESS: user='%s' group='%s'",
						username, gr->gr_name);
					return 1;
				}
			}
		}
	}

	TLOCK_WARNING("passwd: UNLOCK_FAILED: user='%s' not in any allowed group", username);
	return 0;
}

struct aAuth tlock_auth_passwd = {
	"passwd",
	tlock_auth_passwd_init,
	tlock_auth_passwd_auth,
	tlock_auth_passwd_deinit
};
