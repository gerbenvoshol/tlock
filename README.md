# Transparent Screenlock Program for Enterprise Linux

This repository hosts a Transparent Screenlock Program tailored for Linux environments. It's particularly useful in kiosk or operational settings where controlled access to the screen is essential. Installations are configured with a predefined list of groups permitted to unlock the screen on each computer, with comprehensive logging to track screen unlock activities.

Inspired by e-motional.com's Transparent Screen Lock Security software designed for Windows Systems, this program has been adapted and optimized for Linux systems.

## Features
- **Transparent Locking**: A grey border appears around the desktop when the screen is locked.
- **User Authentication**: Unlocking the screen requires users to input their username and password.
- **User Groups Configuration**: Administrators can specify which groups have permission to unlock the screen.
- **Logging**: Detailed logs document screen unlock events, providing accountability and oversight.

## Installation
Ensure the following dependencies are installed:
- `libxt-dev`
- `libpam-dev`
- `GCC` (GNU Compiler Collection)
- Autotools

Additionally, install `xautolock`.

After installation, configure lock interval and user groups permitted to unlock the screen by editing `/etc/X11/xinit/xinitrc.d/60tlock.sh`.

## Usage
When the screen is locked, follow these steps to unlock it:
1. Press any key to bring up the unlock dialogue.
2. Input your username.
3. Press the tab key or click to navigate to the Password field, and enter your password.
4. Press the enter key or click the Login button to unlock the screen.
5. The clear button will remove entries in the Username and Password fields.
6. The cancel button will exit the unlock dialogue.

## License
This program is distributed under the GNU General Public License v2.0 (GPLv2), ensuring its continued availability and openness for customization and improvement.
