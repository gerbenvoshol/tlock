# Transparent Screen Lock (tlock)

A transparent screen-lock program for Linux X11 environments, originally developed at Cornell University. Useful in kiosk or operational settings where controlled, group-based access to the screen is required.

Inspired by e-motional.com's Transparent Screen Lock for Windows, adapted and extended for modern Linux.

## Features

- **Transparent frame**: A coloured border (grey → green while typing → red on failure) wraps the desktop when locked.
- **PAM authentication**: Uses the system PAM stack (`login` service by default, configurable at build time).
- **Group-based authorization**: Only users that belong to specified groups can unlock the screen.
- **Multi-monitor support**: Automatically detects the primary monitor via XRandR and centres the dialog on it.
- **Syslog logging**: Unlock attempts (success and failure) are written to the system log.
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
- XRandR (`libxrandr`) – for multi-monitor primary-screen detection
- PAM (`libpam`)
- **xautolock** – to trigger locking after an idle period (optional but recommended)

### Build
- GCC (or any C99 compiler)
- Autotools (`autoconf`, `automake`)
- Development headers:
  - Debian / Ubuntu: `libx11-dev libxrandr-dev libxt-dev libpam0g-dev`
  - RHEL / Fedora / CentOS: `libX11-devel libXrandr-devel libXt-devel pam-devel`

---

## Installation

### Debian / Ubuntu

```bash
# Install build dependencies
sudo apt-get install -y gcc autoconf automake \
    libx11-dev libxrandr-dev libxt-dev libpam0g-dev xautolock

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
    libX11-devel libXrandr-devel libXt-devel pam-devel xautolock

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
sudo pacman -S gcc autoconf automake libx11 libxrandr libxt pam xautolock

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

### Command-line options

| Option | Description |
|---|---|
| `-auth <module>[,args]` | Authentication module: `none`, `pam`, `xspam,group1,group2,...` |
| `-gids` | Interpret group specifiers in xspam as numeric GIDs (default: group names) |
| `-test` | Test mode: show dialog without grabbing keyboard/pointer |
| `-pre` | Pre-authorisation check: exit immediately if the current user can already unlock |
| `-flash` | Flash the border colour while idle |
| `-bg <type>` | Background module (currently only `none`) |
| `-cursor <type>` | Cursor module (currently only `none`) |
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

