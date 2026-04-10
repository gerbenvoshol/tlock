/*
 ============================================================================
 Name        : tlock_auth_hash.c
 Published   : GNU Public License v2 (GPLv2)
 Description : tlock - authenticate by comparing a password against a
               stored bcrypt hash using crypt(3) from libxcrypt / glibc.

 The hash module is intended for situations where you want to set a single
 shared unlock password without storing it in the system account database.

 Usage: -auth hash,<bcrypt-hash>[,group1[,group2[,...]]]

   The hash must be a bcrypt hash string starting with "$2b$" (or "$2a$" /
   "$2y$").  You can generate one on the command line:

     python3 -c "import bcrypt; print(bcrypt.hashpw(b'secret', bcrypt.gensalt()).decode())"
   or:
     htpasswd -bnBC 12 "" mypassword | tr -d ':\n'

   The optional group list restricts which users can unlock; if omitted any
   user who supplies the correct password is allowed.

 Example:
   tlock -auth hash,'$2b$12$...',techgroup

 Notes:
   - libxcrypt (>=4.4) or glibc >=2.7 provides crypt_r() with bcrypt support.
   - The stored hash is written to the process argument list at startup; on
     production systems pass it via a config file rather than on the command
     line to avoid exposing it in 'ps' output.
 ============================================================================
 */

#include "tlock.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <crypt.h>
#include <sys/types.h>

#define HASH_MAX_GROUPS 50

static char        *hash_stored   = NULL;   /* bcrypt hash string */
static const char  *hash_groups[10];
static int          hash_initialized = 0;

/* ------------------------------------------------------------------ */

static int tlock_auth_hash_init(const char *args)
{
	int   ic = 0;
	char *tmp;
	char *token;

	if (hash_initialized) {
		TLOCK_WARNING("hash: already initialized");
		return 0;
	}

	memset(hash_groups, 0, sizeof(hash_groups));

	if (!args || !*args) {
		TLOCK_ERR("hash: no hash provided – usage: -auth hash,<bcrypt-hash>[,group,...]");
		return 0;
	}

	tmp = strdup(args);

	strtok(tmp, ",");          /* consume module name "hash" */
	token = strtok(NULL, ","); /* first real argument = the hash */
	if (!token || *token == '\0') {
		TLOCK_ERR("hash: missing bcrypt hash argument");
		free(tmp);
		return 0;
	}

	/* Validate: must look like a crypt hash (starts with '$'). */
	if (token[0] != '$') {
		TLOCK_ERR("hash: provided value does not look like a crypt hash");
		free(tmp);
		return 0;
	}

	hash_stored = strdup(token);

	/* Optional group list. */
	for (ic = 0; ic < 10; ic++) {
		hash_groups[ic] = strtok(NULL, ",");
		if (!hash_groups[ic])
			break;
		hash_groups[ic] = strdup(hash_groups[ic]);
		TLOCK_NOTICE("hash: allowed group[%d]=%s", ic, hash_groups[ic]);
	}

	free(tmp);
	hash_initialized = 1;
	return 1;
}

static int tlock_auth_hash_deinit(void)
{
	int i;
	if (hash_stored) {
		/* Overwrite before free to avoid leaving the hash in memory. */
		memset(hash_stored, 0, strlen(hash_stored));
		free(hash_stored);
		hash_stored = NULL;
	}
	for (i = 0; i < 10 && hash_groups[i]; i++) {
		free((void *)hash_groups[i]);
		hash_groups[i] = NULL;
	}
	hash_initialized = 0;
	return 1;
}

/*
 * Compare the supplied password against the stored bcrypt hash, then
 * optionally verify that `username` belongs to an allowed group.
 */
static int tlock_auth_hash_auth(const char *username, const char *pass, int as_gid)
{
	char          *result;
	gid_t          gids[HASH_MAX_GROUPS + 1];
	int            ngroups, i, j;
	struct passwd *pw;
	struct group  *gr;

	if (!pass) {
		TLOCK_WARNING("hash: UNLOCK_FAILED: null password");
		return 0;
	}
	if (!hash_stored) {
		TLOCK_ERR("hash: module not initialized");
		return 0;
	}

	/* crypt(3) with the stored hash as salt performs the comparison. */
	result = crypt(pass, hash_stored);
	if (!result || strcmp(result, hash_stored) != 0) {
		TLOCK_WARNING("hash: UNLOCK_FAILED: bad password for user '%s'",
			username ? username : "(unknown)");
		return 0;
	}

	/* Hash matches. If no group restriction, grant access. */
	if (!hash_groups[0]) {
		TLOCK_NOTICE("hash: UNLOCK_SUCCESS: user='%s' (no group restriction)",
			username ? username : "(unknown)");
		return 1;
	}

	/* Group check requires a username. */
	if (!username || !*username) {
		TLOCK_WARNING("hash: UNLOCK_FAILED: no username supplied for group check");
		return 0;
	}

	pw = getpwnam(username);
	if (!pw) {
		TLOCK_WARNING("hash: UNLOCK_FAILED: unknown user '%s'", username);
		return 0;
	}

	ngroups = HASH_MAX_GROUPS;
	getgrouplist(username, pw->pw_gid, gids, &ngroups);

	for (i = 0; i < ngroups; i++) {
		for (j = 0; j < 10 && hash_groups[j]; j++) {
			if (as_gid) {
				gid_t wanted = (gid_t)atoi(hash_groups[j]);
				if (gids[i] == wanted) {
					TLOCK_NOTICE("hash: UNLOCK_SUCCESS: user='%s' gid=%u",
						username, (unsigned)gids[i]);
					return 1;
				}
			} else {
				gr = getgrgid(gids[i]);
				if (gr && strcmp(gr->gr_name, hash_groups[j]) == 0) {
					TLOCK_NOTICE("hash: UNLOCK_SUCCESS: user='%s' group='%s'",
						username, gr->gr_name);
					return 1;
				}
			}
		}
	}

	TLOCK_WARNING("hash: UNLOCK_FAILED: user='%s' not in any allowed group", username);
	return 0;
}

struct aAuth tlock_auth_hash = {
	"hash",
	tlock_auth_hash_init,
	tlock_auth_hash_auth,
	tlock_auth_hash_deinit
};
