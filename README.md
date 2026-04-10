# Transparent Screen Lock (tlock)

A transparent screen-lock program for Linux X11 environments, originally developed at Cornell University. Designed for shared laboratory computers where different technicians share a single workstation under a general account, and screen locking provides access control between sessions.

Inspired by e-motional.com's Transparent Screen Lock for Windows, adapted and extended for modern Linux.

## Features

- **Transparent frame**: A coloured border (grey → green while typing → red on failure) wraps the desktop when locked.
- **Multiple authentication modules**: PAM (with group control), shadow passwd, bcrypt hash, or none.
- **Multiple background modules**: Transparent (none), solid colour (blank), dimmed overlay (shade), PNG image.
- **Multiple cursor modules**: Unchanged (none), invisible (blank), X11 glyph, Xcursor theme, PNG image.
- **Group-based authorization**: Only users that belong to specified groups can unlock the screen.
- **Multi-monitor support**: Automatically detects the primary monitor via XRandR and centres the dialog on it.
- **GCLP-compliant syslog audit trail**: All lock, unlock, and authentication events are written to syslog. Passwords are never logged.
- **Test mode** (`-test`): Preview the dialog and exercise authentication without actually locking the desktop.

---

## Platform support

| Session type | Status |
|---|---|
| X11 (Xorg) | ✅ Fully supported |
| XWayland (Wayland + compatibility layer) | ⚠️ Partially supported – compositor shortcuts can bypass the keyboard grab. Use a native Wayland locker in production. |
| Pure Wayland (no XWayland) | ❌ Not supported – tlock will refuse to start. Use **swaylock**, **waylock**, or **gtklock** instead. |

---

## Dependencies

### Runtime
- X11 (`libx11`)
- XRandR (`libxrandr`) – multi-monitor primary-screen detection
- XRender (`libxrender`) – `shade` background module
- Xcursor (`libxcursor`) – `xcursor` and `image` cursor modules
- PAM (`libpam`)
- libpng (`libpng`) – `image` background/cursor modules
- libcrypt (`libcrypt`) – `passwd` and `hash` auth modules
- **xautolock** – to trigger locking after an idle period (optional but recommended)

### Build
- GCC (or any C99 compiler)
- Autotools (`autoconf`, `automake`)
- Development headers:
  - Debian / Ubuntu: `libx11-dev libxrandr-dev libxt-dev libxrender-dev libxcursor-dev libpam0g-dev libpng-dev libcrypt-dev`
  - RHEL / Fedora / CentOS: `libX11-devel libXrandr-devel libXt-devel libXrender-devel libXcursor-devel pam-devel libpng-devel libxcrypt-devel`

---

## Installation

### Debian / Ubuntu

```bash
# Install build dependencies
sudo apt-get install -y gcc autoconf automake \
    libx11-dev libxrandr-dev libxt-dev libxrender-dev \
    libxcursor-dev libpam0g-dev libpng-dev libcrypt-dev xautolock

# Build and install
autoreconf -fi
./configure --prefix=/usr/local
make
sudo make install

# Install the autostart script
sudo install -m 755 bin/60tlock.sh /etc/X11/xinit/xinitrc.d/
```

The PAM service defaults to `login`.  On Debian/Ubuntu you may prefer
`common-auth`:

```bash
./configure --prefix=/usr/local \
    CFLAGS="-DPAM_SERVICE_NAME=\\\"common-auth\\\""
```

### RHEL / CentOS / Fedora

```bash
# Install build dependencies
sudo dnf install -y gcc autoconf automake \
    libX11-devel libXrandr-devel libXt-devel libXrender-devel \
    libXcursor-devel pam-devel libpng-devel libxcrypt-devel xautolock

# Build and install
autoreconf -fi
./configure --prefix=/usr/local
make
sudo make install

# Install the autostart script
sudo install -m 755 bin/60tlock.sh /etc/X11/xinit/xinitrc.d/
```

On RHEL/CentOS the default PAM service name (`login`) should work.
If your site uses `system-auth`:

```bash
./configure --prefix=/usr/local \
    CFLAGS="-DPAM_SERVICE_NAME=\\\"system-auth\\\""
```

### Arch Linux

```bash
sudo pacman -S gcc autoconf automake libx11 libxrandr libxt libxrender \
    libxcursor pam libpng xautolock

autoreconf -fi
./configure --prefix=/usr/local
make
sudo make install
sudo install -m 755 bin/60tlock.sh /etc/X11/xinit/xinitrc.d/
```

---

## Configuration

Edit `/etc/X11/xinit/xinitrc.d/60tlock.sh` and set the groups allowed to unlock:

```bash
UNLOCK_GROUPS="mygroup,root"
```

The script launches `xautolock` (if available) to lock after 15 minutes of
idle time, and triggers an immediate lock on login.

---

## Usage

### Normal operation

```bash
# Lock using PAM + group authorisation (numeric GIDs)
tlock -auth xspam,500,0 -gids

# Lock using PAM + group authorisation (group names)
tlock -auth xspam,mygroup,root

# Lock with dim background + invisible cursor
tlock -auth xspam,mygroup,root -bg shade:0.6 -cursor blank
```

### Test mode (no keyboard/pointer grab)

Use `-test` to preview the dialog without locking the desktop.  Useful during
initial configuration and CI/CD pipelines:

```bash
tlock -auth none -test
```

The dialog is shown immediately.  Keyboard focus is set to the dialog window
so you can navigate with Tab/Enter/Escape.  Press **Cancel** or **Escape** to
exit.  Because there is no keyboard grab, you can still switch to other
windows normally.

### Authentication flow

1. When the screen is locked press any key to open the unlock dialog.
2. Enter your **Username**.
3. Press **Tab** (or click) to move to the **Password** field.
4. Press **Enter** or click **Login** to authenticate.
5. **Clear** empties both fields.
6. **Cancel** closes the dialog (screen remains locked).

---

## Module reference

### Authentication modules (`-auth`)

| Module | Description |
|---|---|
| `none` | No authentication – anyone can unlock. Not recommended. |
| `pam` | Authenticate against the PAM stack (`login` service). See PAM service name section. |
| `xspam[,group1,...]` | PAM authentication **with group membership check**. Only members of the listed groups can unlock. Use `-gids` to specify numeric GIDs instead of group names. |
| `passwd[,group1,...]` | Authenticate against `/etc/shadow` using crypt(3). Optional group restriction. Requires tlock to be `setgid shadow` or run as root. |
| `hash,<bcrypt-hash>[,group1,...]` | Verify password against a pre-computed crypt/bcrypt hash. Useful for shared unlock passwords without a dedicated system account. Optional group restriction. |

#### Generating a hash for the `hash` module

```bash
# Using Python (bcrypt)
python3 -c "import bcrypt; print(bcrypt.hashpw(b'mypassword', bcrypt.gensalt(12)).decode())"

# Using openssl (SHA-512 crypt)
openssl passwd -6 mypassword
```

Then pass the hash as the first argument:
```bash
tlock -auth "hash,\$6\$salt\$hashedvalue,techgroup"
```

### Background modules (`-bg`)

| Module | Description |
|---|---|
| `none` | Transparent background (default) – desktop content remains visible. |
| `blank[:<color>]` | Fill all screens with a solid colour. Default: `black`. Accepts X11 named colours or `#RRGGBB` hex. Example: `-bg blank:#1a1a2e` |
| `shade[:<opacity>]` | Dim the screen with a semi-transparent black overlay via X RENDER. Opacity in `[0.0, 1.0]`, default `0.5`. Example: `-bg shade:0.7` |
| `image:<path>[:<mode>]` | Display a PNG image. Modes: `scale` (default), `tile`, `center`. Example: `-bg image:/etc/tlock/bg.png:scale` |

### Cursor modules (`-cursor`)

| Module | Description |
|---|---|
| `none` | Leave the cursor unchanged (default). |
| `blank` | Fully invisible cursor (1×1 transparent bitmap). |
| `glyph[:<index>]` | X11 cursor-font glyph. Default: 150 (XC_watch). See `<X11/cursorfont.h>` for all indices. |
| `xcursor[:<name>]` | Load a cursor from the current Xcursor theme. Default: `watch`. Example: `-cursor xcursor:left_ptr` |
| `image:<path>[:<hx>,<hy>]` | Load a PNG (RGBA) as the cursor. `hx,hy` = hotspot offset (default: `0,0`). Example: `-cursor image:/etc/tlock/cursor.png:4,4` |

### Command-line options

| Option | Description |
|---|---|
| `-auth <module>[,args]` | Authentication module (see above) |
| `-bg <module>[:<args>]` | Background module (see above) |
| `-cursor <module>[:<args>]` | Cursor module (see above) |
| `-gids` | Interpret group specifiers as numeric GIDs (default: group names) |
| `-test` | Test mode: show dialog without grabbing keyboard/pointer |
| `-pre` | Pre-authorisation check: skip lock if current user is already in the allowed group |
| `-flash` | Flash the border colour while idle |
| `-v` | Print version |
| `-h` | Print usage |

---

## PAM service name

The compiled-in default is `login`, which exists on all major distributions.
Override at build time with:

```bash
./configure CFLAGS="-DPAM_SERVICE_NAME=\\\"<service>\\\""
```

Common values:

| Distribution | PAM service |
|---|---|
| RHEL / CentOS | `system-auth` |
| Debian / Ubuntu | `common-auth` |
| Arch Linux | `system-local-login` |
| Most others | `login` |

---

## Audit logging (GCLP compliance)

tlock writes all security-relevant events to `syslog` using the `LOG_LOCAL1`
facility.  This ensures events appear in the system audit trail and can be
forwarded to a central log server (e.g. via rsyslog or journald).

### Log event format

| Keyword | Level | Meaning |
|---|---|---|
| `SCREEN_LOCKED` | NOTICE | Screen was locked (recorded at lock time) |
| `SCREEN_UNLOCKED` | NOTICE | Lock session ended |
| `UNLOCK_SUCCESS` | NOTICE | User authenticated and was granted access |
| `UNLOCK_FAILED` | WARNING | Authentication attempt failed |
| `UNLOCK_LOCKED_OUT` | ALERT | Too many failed attempts (`-DATTEMPT_LIMIT=n`) |
| `PRECHECK` | NOTICE | Pre-authorisation check result |

### Sample log lines

```
Apr 10 14:00:01 lab-pc01 tlock[1234]: SCREEN_LOCKED: uid=1000 user=labuser auth=xspam groups_as=name
Apr 10 14:05:23 lab-pc01 tlock[1234]: UNLOCK_FAILED: user=johndoe auth=xspam
Apr 10 14:05:41 lab-pc01 tlock[1234]: UNLOCK_SUCCESS: user=johndoe auth=xspam group=techgroup
Apr 10 14:05:41 lab-pc01 tlock[1234]: SCREEN_UNLOCKED: uid=1000 user=labuser
```

**Passwords are never logged.**  The username is always recorded to provide a
traceable audit record (which technician unlocked the workstation and when).

### Configuring rsyslog to capture tlock events

```
# /etc/rsyslog.d/tlock.conf
local1.*   /var/log/tlock.log
```

---

## Wayland

tlock is an **X11 application** and requires an X display (`$DISPLAY`).

- **XWayland**: tlock will start and the dialog will appear, but the
  Wayland compositor can still dispatch keyboard shortcuts (e.g. `Super`,
  virtual terminal switching).  For workstations this may be acceptable; for
  high-security kiosks use a native Wayland locker.
- **Pure Wayland** (no `$DISPLAY`): tlock prints an error and exits.

Recommended native Wayland lockers: **swaylock** (sway/wlroots),
**waylock**, **gtklock** (GTK-based, multi-seat).

---

## License

GNU General Public License v2.0 (GPLv2) — see `LICENSE`.

