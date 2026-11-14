# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

spkg is a C replacement for Slackware's pkgtools (`installpkg`/`upgradepkg`/`removepkg`), tuned for speed. It shares the pkgtools package database, so the two can be used side by side. It is public domain.

## Build

Autotools project. Dependencies: Judy, glib-2.0, popt, zlib.

```sh
./autogen.sh            # runs autoreconf, then ./configure --enable-maintainer-mode "$@"
make                    # builds src/libspkg.a and src/spkg
```

Configure options that change the code:
- `--enable-static-spkg` builds `spkg.static` alongside the dynamic `spkg`. `--enable-static-spkg=only` builds only a static `spkg` (used by `spkg.SlackBuild`) and skips installing libspkg/pkg-config.
- `--enable-assume-broken-pkgdb` defines `ASSUME_BROKEN_PKGDB`, which normalizes `//` paths in the package DB.
- `--disable-legacy` turns off `LEGACY_CHECKS` (for example, the check that rejects symlinks inside the archive).

## Tests

```sh
make check                              # build and run all tests in tests/
make check TESTS=test-parsers           # run a single test
cat tests/test-parsers.log              # output of a test (failed checks print FAILED file:line)
make distcheck                          # full release check: builds the tarball and runs the tests from it
```

Tests are plain C programs linked against `src/libspkg.a`, listed in `TESTS` / `check_PROGRAMS` in `tests/Makefile.am`. Each one exits non-zero on failure. Use the `CHECK`, `CHECK_STR` and `CHECK_INT` macros from `tests/check.h` and return `check_result()` from `main()`. Tests that touch the filesystem must work inside `check_enter_tmpdir()` and remove it at the end, never in the build tree or `/`. Fixture files (such as `tests/blacklist`) are read from `$srcdir` so VPATH builds and distcheck work, and must be added to `EXTRA_DIST`.

`tests/test-commands.sh` is the end-to-end suite: it builds small packages with tar, then runs the real `src/spkg` against a temporary `--root` under `fakeroot` (it skips if fakeroot is missing). `VERBOSE=1` shows spkg's output, and `KEEP=1` keeps the temporary directory. Known bugs are checked with `xfail`. These report XFAIL while the bug exists and fail with XPASS once it's fixed, so the check can be turned into a normal `check`.

`test-untgz <pkg.tgz>...` is a manual tool (it extracts into the current directory) built by `make check` but not run. `test-pkgtools-speed.sh` benchmarks spkg against pkgtools.

To test install/upgrade/remove without touching the real system, use an alternate root (`-r <dir>`) and/or dry-run mode. `.gdbinit` (`spkg -v -r .root -i <pkg>`) and `.valgrindrc` show the debugging setup the developers use.

## Architecture

Nearly everything lives in `libspkg.a`. `src/main.c` only parses options with popt and dispatches to `cmd_install`, `cmd_upgrade`, `cmd_remove`, or `cmd_list` (`include/commands.h`), passing a shared `struct cmd_options`. Public headers are in `include/`. `src/cmd-private.h` holds helpers used only by the command files.

Core modules:
- **untgz** (`untgz.c`) is a streaming tar reader with its own built-in decompression. Commands must extract the package in **one pass**: they iterate headers with `untgz_get_header()` and decide per entry what to do.
- **pkgdb** (`pkgdb.c`) reads and writes the pkgtools database under `<root>/var/lib/pkgtools/{packages,scripts}` and `<root>/var/log/pkgtools/removed_*`. A package's file list is held in Judy arrays (`JSL*` macros, `pkg->paths`) for fast path lookups. The DB uses a lock, so `DB_BLOCKED` means another process holds it.
- **taction** (`taction.c`) is the filesystem transaction layer that provides rollback. Commands register deferred actions with `ta_<on_finalize>_<on_rollback>()` (for example `ta_move_remove`, `ta_keep_remove`), then call `ta_finalize()` (runs actions in order) or `ta_rollback()` (runs them in reverse). Changes up to the end of archive extraction must stay rollback-able. New files are extracted to temporary names and moved into place on finalize. Dry-run is implemented here: in dry-run, finalize and rollback don't touch the filesystem.
- **sigtrap** sets `sig_break` on signals. Commands poll it via the `_safe_breaking_point(label)` macro so they can cancel safely and roll back.
- **error** (`error.c`) is an error object passed as `struct error* e` through every API. Use `e_set`/`e_add` to set errors. Module-specific codes are built with `E(n)` (for example `DB_NOTEX`, `CMD_EXIST`). Functions return 0 on success and non-zero on error.
- **message** provides `_notice`/`_warning`/etc. output gated by verbosity.
- **cmd-common** (`cmd-common.c`) holds shared command logic:
  - It reads `doinst.sh` and optimizes away its symlink-creation lines. spkg creates those links itself, unless `--no-optsyms` is given or the package is on the `bl_symopts` blacklist. The full script is still stored in the DB.
  - It runs `doinst.sh`, ldconfig, and gtk-update-icon-cache.
  - It handles `douninst.sh` scripts (`var/lib/pkgtools/douninst.sh/`), which run on remove and upgrade.
- **cmd-upgrade** combines install of the new package with removal of the old package's leftover files, all inside one transaction.

Code is GLib-based (`gchar`, `GSList`, `g_*` allocation) and documented with Doxygen-style comments. The man page is `docs/spkg.8`. Update it when you add or change CLI options. Release notes go in `NEWS`, and the version number is set in `configure.ac`.

## Style

2-space indentation, no tabs, braces on their own line (see the vim modeline in the sources). Each file starts with the standard spkg header comment block.
