# spkg

## About

The unofficial Slackware Linux package manager. It is a replacement for the
standard Slackware pkgtools (installpkg/upgradepkg/removepkg). It is
implemented in C and optimized for speed.

## Features

* Simple user interface. Just like pkgtools.
* Fast install, upgrade and remove operations.
* Command to list information about installed packages.
* Dry-run mode, in which filesystem is not touched.
* Safe mode for installing untrusted packages.
* Rollback and safe cancel functionality.
* You can use spkg and pkgtools side by side.
* You can be informed about all actions spkg does if you turn on verbose mode.
* Everything is libified. (see docs) You can implement new commands easily.

## License

The license is Public Domain Software. See COPYING file.

## Download

* https://github.com/gapan/spkg/releases

## Issues/Feedback

* https://github.com/gapan/spkg/issues

## Credits

Current Maintainer: George Vlahavas <vlahavas~at~gmail~dot~com>
Original Author: Ondrej Jirman <megous@megous.com>

