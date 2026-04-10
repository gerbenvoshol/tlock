#!/bin/bash
# ============================================================================
# Name        : 60tlock.sh
# Author      : akaan
# Version     : 1.1
# Copyright   : Cornell University - cornell.edu
# Description : tlock - transparent lock, Ansi-style
# Updated     : 2024 – modernised for systemd/XDG, Wayland guard, modern distros
#============================================================================
#
# Usage: drop script in /etc/X11/xinit/xinitrc.d/ and configure the groups
#        that are allowed to unlock the screen below.
#
# The script is sourced by the X11 session initialisation and runs as the
# login user.  It is NOT executed under Wayland sessions.
#

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------

# Comma-separated list of group names (or numeric GIDs when using -gids).
# Only members of these groups can unlock the screen.
UNLOCK_GROUPS="cmpgrp,root"

# Idle timeout in minutes before the screen locks automatically.
LOCK_TIMEOUT=15

# Paths
TLOCK="${TLOCK:-/usr/local/bin/tlock}"
XAUTOLOCK="${XAUTOLOCK:-$(command -v xautolock 2>/dev/null)}"

# ----------------------------------------------------------------------------
# Sanity checks
# ----------------------------------------------------------------------------

# tlock is an X11 application – skip silently under Wayland.
if [ -n "$WAYLAND_DISPLAY" ] && [ -z "$DISPLAY" ]; then
    echo "60tlock.sh: Wayland session without XWayland – skipping tlock." >&2
    exit 0
fi

if [ ! -x "$TLOCK" ]; then
    echo "60tlock.sh: tlock not found at $TLOCK – not starting." >&2
    exit 0
fi

# ----------------------------------------------------------------------------
# Start
# ----------------------------------------------------------------------------

TLOCK_CMD="$TLOCK -auth xspam,${UNLOCK_GROUPS}"

if [ -n "$XAUTOLOCK" ] && [ -x "$XAUTOLOCK" ]; then
    # Start xautolock to trigger locking after idle, then lock immediately.
    "$XAUTOLOCK" -noclose -time "$LOCK_TIMEOUT" -locker "$TLOCK_CMD" &
    sleep 1 && "$XAUTOLOCK" -locknow &
else
    # No xautolock – lock once immediately.
    $TLOCK_CMD &
fi

