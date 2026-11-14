# Differences between spkg and pkgtools

spkg shares the package database with pkgtools (`installpkg`, `upgradepkg`
and `removepkg`), so the two can be used on the same system. This file
lists where spkg behaves differently. It's compared with the pkgtools of
Slackware 15.0.

## Commands and options

One program does everything:

| pkgtools | spkg |
|---|---|
| `installpkg pkg.txz` | `spkg -i pkg.txz` |
| `upgradepkg pkg.txz` | `spkg -u pkg.txz` |
| `upgradepkg --install-new pkg.txz` | `spkg -u --install-new pkg.txz`, or just `spkg pkg.txz` |
| `upgradepkg --reinstall pkg.txz` | `spkg -u --reinstall pkg.txz` |
| `removepkg name` | `spkg -d name` |
| (none) | `spkg -l [glob...]` lists installed packages, `-l -v` with details |

The root can be given with `--root` or the `ROOT` environment variable,
as with pkgtools.

Options that spkg has and pkgtools doesn't:

- `-n`, `--dry-run` for every command (pkgtools has `upgradepkg --dry-run`
  and the `--warn` options of installpkg and removepkg).
- `-s`, `--safe`: don't replace existing files when installing or
  upgrading (installpkg's `--no-overwrite` is similar), don't run install
  scripts, and don't remove files that were changed after installation.
- `-f`, `--force`: install a package even if another version with the
  same base name is installed (installpkg always allows that).
- `-p`, `--purge`: when removing, also remove the configuration files
  that were installed as `.new` and renamed by the install script.
- `--no-scripts`: don't run `doinst.sh` or `douninst.sh` (removepkg's
  `--skip-douninst` covers only the second).
- `--no-fast-symlinks`, `--no-ldconfig`, `--no-gtk-update-icon-cache`, see
  below.
- `-v`/`-vv` and `-q`/`-qq` set how much is printed.

pkgtools options spkg doesn't have:

- installpkg: `--md5sum`, `--terse`, `--terselength`, `--ask`, `--menu`,
  `--infobox`, `--tagfile`, `--priority`, `--threads`, `--warn`,
  `--no-overwrite` (see `--safe`).
- upgradepkg: `old%new` to replace a package with one of a different
  name, `--no-paranoia`, `--terse`, `--terselength`, `--verbose`. A rename can be done
  as `spkg -i new.txz` followed by `spkg -d old`: files both packages
  have stay, because the new package owns them too. (Removing the old one
  first would leave a gap.)
- removepkg: `--copy`, `--keep`, `--preserve`, `--warn`.

## Package formats

| Extension | pkgtools | spkg |
|---|---|---|
| `.tgz` | gzip | gzip, decompressed by spkg itself (zlib) |
| `.txz` | xz | xz, through `xzdec` |
| `.tlz` | lzip (`plzip` if installed) | lzip (`plzip` if installed) |
| `.tbz` | bzip2 (`lbzip2` if installed) | bzip2 (`lbzip2` if installed) |
| `.tbr` | not supported | brotli, through `brotli` |
| `.tar` | not supported | uncompressed tar |

- The archive must be GNU or ustar tar, as makepkg writes it. pax archives
  (`tar --format=pax`) are rejected as corrupt.
- If the decompressor reports an error at the end of the archive, the
  package is rejected and the install or upgrade is rolled back. xz checks
  its data only after writing it, so corruption in a file's data shows up
  only then; installpkg prints xz's error but installs the damaged
  package. Brotli has no checksum, so corruption in `.tbr` packages can go
  unnoticed.
- With the default build, a symlink inside the archive makes the package
  fail ("Symlink was found in the archive"): makepkg turns symlinks into
  `doinst.sh` lines, so one in the archive means a broken package. Built
  with `--disable-legacy`, spkg installs it.

## Installing

- **Transactions.** spkg reads the package once and keeps everything up to
  the database update undoable. A broken package, an error or Ctrl-C rolls
  the install back: new files are removed, replaced files restored.
  installpkg has no rollback.
- **Symlinks from doinst.sh.** spkg takes the `( cd dir ; rm -rf link )`
  and `( cd dir ; ln -sf target link )` lines out of `doinst.sh`, creates
  those symlinks itself during the install, and runs only the rest of the
  script. This is much faster for packages with many symlinks. The whole
  script is still stored in the database, as installpkg does.
  - `--no-fast-symlinks` turns this off: the script runs as it is.
  - It is always off for `aaa_base`, `aaa_glibc-solibs`, `glibc-solibs`
    and `glibc`, whose scripts must run exactly as written (glibc swaps
    the running libraries and makes its symlinks only after copying them
    into place). Packages listed in `/etc/spkg/symopts_blacklist` (one base
    name per line) are added to this list.
  - In both cases the symlinks the script makes are still recorded, so
    removing or upgrading the package handles them like pkgtools does.
- **Running doinst.sh.** spkg runs the script with `/bin/sh` from the root
  directory, without arguments, after all files are in place.
  installpkg pipes it through `sed` (turning `( cd x ; ... )` subshells into
  `pushd`/`popd`) to `/bin/bash`, under a lock in `/run/lock/pkgtools`
  unless the script contains `NOLOCK`. spkg takes no such lock.
- **The description** comes from `install/slack-desc` in the package.
  installpkg first looks for a `<name>.txt` file next to the package.
- **File owners** are set from the numeric uid and gid in the archive (like
  `tar --numeric-owner`). installpkg's `tar -xp` prefers the user and group
  names. This only matters if a name maps to a different number on the
  target system.
- **A symlink in the way of a directory.** If the package has a directory
  where the system has a symlink to a directory, spkg installs into the
  directory behind it, with a warning (like `tar --keep-directory-symlink`
  in installpkg). If that symlink belonged to the version being upgraded,
  spkg replaces it with a real directory instead; installpkg's first pass
  writes through it.
- **Existing files** are replaced with a warning ("File already exist …
  it will be replaced"); installpkg replaces them silently.
- **The package sizes** in the database entry are whole kilobytes, rounded
  down: the size of the package file and the sum of the sizes of the files
  in it. installpkg writes `du -h` of the package and the size of the tar
  stream (`4.0K`, `1.2M`). Both are read by both.
- `install/` (with `doinst.sh`) is removed from the root after the install.

## Upgrading

- **One transaction.** spkg extracts the new version, removes the files only
  the old version had, replaces the database entry, runs the old version's
  `douninst.sh`, and then the new `doinst.sh` once.
  upgradepkg renames the old entry to `<name>-upgraded-<time>`, installs the
  new package without running its script, runs removepkg on the old entry,
  and installs the new package a second time to run the script (unless
  `--no-paranoia`).
- **The libc symlinks** (`ld-linux*`, `libc.so*`, `libdl.so*` in `lib` and
  `lib64`) are never removed as leftovers, so the shell keeps working until
  the new install script makes them again.
- **Files changed after the installation** (newer than the database entry)
  are removed with a warning; with `--safe` they are kept.
- **Other installed versions** with the same base name are removed after
  the upgrade, as with upgradepkg.

## Removing

- **Files changed after the installation** are removed with a warning; with
  `--safe` they are kept. `--purge` also removes renamed configuration files.
- **The removed entries** in `var/log/pkgtools/removed_packages` and
  `removed_scripts` are named `<name>-removed-<time>`, by remove and by
  upgrade. removepkg uses the plain package name, and upgradepkg's removal
  `<name>-upgraded-<time>`. The uninstall script in
  `removed_uninstall_scripts` keeps the plain package name with spkg.
- **Configuration files** that the install script renamed from `.new` give
  a "File was already removed" warning; removepkg says nothing.

## Helper programs

- **ldconfig** runs at most once per spkg command, after the last package,
  if any package installed, upgraded or removed a file named `*.so` or
  `*.so.*`. It runs as `ldconfig -r <root>`, also for other roots, and only
  if `<root>/etc/ld.so.conf` exists. installpkg and removepkg run ldconfig
  after every package, but only when the root is `/`.
- **gtk-update-icon-cache** runs on `<root>/usr/share/icons/hicolor` after
  installing or upgrading a package with a `.desktop` file. pkgtools never
  runs it (packages do it in their install scripts).

## Package database

- spkg locks the database with `var/lib/pkgtools/packages/.lock` while it
  changes it, so two spkg commands can't run at once. A second one waits
  about two seconds, then fails. pkgtools doesn't use this lock, so spkg
  and pkgtools must not change the database at the same time. Listing and
  `--dry-run` don't take the lock.
- When a short name matches several installed versions, the most recently
  installed one is used, as with removepkg.
- spkg creates the `var/log/{packages,scripts,setup,removed_packages,
  removed_scripts,removed_uninstall_scripts}` compatibility symlinks
  where nothing exists yet, like installpkg and removepkg together.
- The copy of an uninstall script is kept in
  `var/lib/pkgtools/setup/tmp/spkg/` while it runs (removepkg uses
  `var/lib/pkgtools/setup/tmp/`).
- In read-only mode (`--dry-run`, `-l`, or not running as root) spkg needs
  only `var/lib/pkgtools/packages/` to exist, and never creates the
  database.
