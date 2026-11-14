#!/bin/sh
#/----------------------------------------------------------------------\#
#| spkg - The Unofficial Slackware Linux Package Manager                |#
#|----------------------------------------------------------------------|#
#|          No copy/usage restrictions are imposed on anybody.          |#
#\----------------------------------------------------------------------/#
#
# End-to-end tests for the install, upgrade, remove and list commands.
#
# Small packages are built on the fly with tar and installed into a
# temporary alternate root with the real spkg binary. The script re-runs
# itself under fakeroot, since spkg refuses to modify the system unless
# it runs as root.
#
# Usage: test-commands.sh            (from tests/ in the build dir)
#        SPKG=/path/to/spkg KEEP=1 test-commands.sh
#
# KEEP=1 keeps the temporary directory for inspection.
#
# Checks of known bugs use xfail: they report XFAIL while the bug exists
# and fail the run with XPASS once it's fixed, so the check can be turned
# into a normal one.

set -u

SPKG=${SPKG:-../src/spkg}
if [ ! -x "$SPKG" ]; then
  echo "spkg binary not found: $SPKG"
  exit 99
fi
SPKG=$(cd "$(dirname "$SPKG")" && pwd)/$(basename "$SPKG")

if [ "$(id -u)" != 0 ]; then
  if ! command -v fakeroot >/dev/null 2>&1; then
    echo "SKIP: fakeroot is required to run this test as a regular user"
    exit 77
  fi
  SPKG_REAL_UID=$(id -u)
  export SPKG SPKG_REAL_UID
  exec fakeroot -- /bin/sh "$0" "$@"
fi
SPKG_REAL_UID=${SPKG_REAL_UID:-0}

# Ignore any ROOT from the environment, it would override --root defaults.
unset ROOT

TMP=$(mktemp -d "${TMPDIR:-/tmp}/spkg-test-XXXXXX") || exit 99
if [ "${KEEP:-0}" = 1 ]; then
  echo "keeping $TMP"
else
  trap 'rm -rf "$TMP"' EXIT
fi
PKGS=$TMP/pkgs
mkdir "$PKGS"

failures=0
CASE=

# ---------------------------------------------------------------------
# test helpers
# ---------------------------------------------------------------------

begin() {
  CASE=$1
  echo "== $CASE"
}

fail() {
  echo "FAILED: $CASE: $*"
  failures=$((failures + 1))
}

# check DESCRIPTION COMMAND... : COMMAND must succeed
check() {
  _desc=$1; shift
  "$@" || fail "$_desc"
}

# xfail DESCRIPTION COMMAND... : known bug, COMMAND is expected to fail
xfail() {
  _desc=$1; shift
  if "$@"; then
    echo "XPASS: $CASE: $_desc (known bug fixed? turn this into a check)"
    failures=$((failures + 1))
  else
    echo "XFAIL: $CASE: $_desc"
  fi
}

# fresh, empty alternate root
new_root() {
  ROOT=$TMP/root-$1
  rm -rf "$ROOT"
  mkdir -p "$ROOT"
}

# run spkg on $ROOT; sets $rc, output goes to $OUT
OUT=$TMP/out
spkg() {
  "$SPKG" --root="$ROOT" "$@" >"$OUT" 2>&1
  rc=$?
  if [ "${VERBOSE:-0}" = 1 ]; then
    sed 's/^/    | /' "$OUT"
  fi
}

rc_is()      { [ "$rc" = "$1" ] || { echo "    exit status $rc, want $1"; sed 's/^/    | /' "$OUT"; return 1; }; }
output_has() { grep -q -- "$1" "$OUT" || { echo "    output lacks: $1"; sed 's/^/    | /' "$OUT"; return 1; }; }
is_file()    { [ -f "$ROOT/$1" ] && [ ! -L "$ROOT/$1" ]; }
is_dir()     { [ -d "$ROOT/$1" ] && [ ! -L "$ROOT/$1" ]; }
absent()     { [ ! -e "$ROOT/$1" ] && [ ! -L "$ROOT/$1" ]; }
content_is() { [ "$(cat "$ROOT/$1" 2>/dev/null)" = "$2" ]; }
mode_is()    { [ "$(stat -c %a "$ROOT/$1")" = "$2" ]; }
link_to()    { [ -L "$ROOT/$1" ] && [ "$(readlink "$ROOT/$1")" = "$2" ]; }
db_has()     { [ -f "$ROOT/var/lib/pkgtools/packages/$1" ]; }
db_lacks()   { [ ! -e "$ROOT/var/lib/pkgtools/packages/$1" ]; }
db_lists()   { grep -qx -- "$2" "$ROOT/var/lib/pkgtools/packages/$1"; }
removed_entry() { ls "$ROOT/var/log/pkgtools/removed_packages/$1"-* >/dev/null 2>&1; }

# Everything about the tree under $ROOT: paths, types, modes, sizes, link
# targets and file contents. Used to check that nothing changed.
snapshot() {
  (cd "$ROOT" && find . -printf '%p %y %m %s %l\n' | LC_ALL=C sort &&
    find . -type f -exec md5sum {} + | LC_ALL=C sort)
}

# pack STAGEDIR PKGFILE [MEMBERS...]
# Create a package from STAGEDIR. The compression comes from the PKGFILE
# extension. Entries are named like makepkg does: "./" and then paths
# without a "./" prefix ("install/doinst.sh"). With MEMBERS, only those are
# archived, as given, in that order and without recursing into directories.
pack() {
  _dir=$1 _file=$2; shift 2
  case $_file in
    *.tgz) _comp="gzip -9" ;;
    *.txz) _comp="xz" ;;
    *.tlz) _comp="lzma" ;;
    *.tbr) _comp="brotli -c" ;;
    *.tar) _comp="cat" ;;
  esac
  if [ $# -eq 0 ]; then
    (cd "$_dir" && tar --owner=root --group=root \
      --transform 's,^\./\(.\),\1,S' -cf - .) | $_comp > "$PKGS/$_file"
  else
    (cd "$_dir" && tar --owner=root --group=root -P --no-recursion -cf - "$@") \
      | $_comp > "$PKGS/$_file"
  fi
}

# The main test package. Version 1 and 2 differ in:
# - usr/bin/hello content
# - usr/lib/libhello.so.N and the libhello.so symlink pointing to it
# - usr/share/hello/v1.txt is replaced by usr/share/hello/v2.txt
# Both have a config file handled the usual way by doinst.sh, and an
# uninstall script. doinst.sh and douninst.sh append to script-log, the
# latter with the usr/bin/hello it sees, to check when each one runs.
make_hello() { # make_hello VERSION [EXT]
  _v=$1 _ext=${2:-tgz}
  _st=$TMP/stage-hello-$_v
  rm -rf "$_st"
  mkdir -p "$_st/install" "$_st/usr/bin" "$_st/usr/lib" "$_st/usr/share/hello" \
           "$_st/etc"
  echo "hello $_v" > "$_st/usr/bin/hello"
  chmod 755 "$_st/usr/bin/hello"
  echo "lib $_v" > "$_st/usr/lib/libhello.so.$_v"
  echo "data $_v" > "$_st/usr/share/hello/v$_v.txt"
  echo "setting=$_v" > "$_st/etc/hello.conf.new"
  cat > "$_st/install/slack-desc" <<EOF
hello: hello (test package)
hello:
hello: A package used by the spkg test suite.
EOF
  cat > "$_st/install/doinst.sh" <<EOF
config() {
  NEW="\$1"
  OLD="\$(dirname \$NEW)/\$(basename \$NEW .new)"
  if [ ! -r \$OLD ]; then
    mv \$NEW \$OLD
  elif [ "\$(cat \$OLD | md5sum)" = "\$(cat \$NEW | md5sum)" ]; then
    rm \$NEW
  fi
}
config etc/hello.conf.new
echo "doinst \$1 $_v" > hello-doinst-ran
echo "doinst $_v" >> script-log
( cd usr/lib ; rm -rf libhello.so )
( cd usr/lib ; ln -sf libhello.so.$_v libhello.so )
EOF
  cat > "$_st/install/douninst.sh" <<EOF
echo "douninst $_v saw \$(cat usr/bin/hello 2>/dev/null)" >> script-log
EOF
  pack "$_st" "hello-$_v-noarch-1.$_ext"
}

# the script-log lines, joined with ";"
script_log() { cat "$ROOT/script-log" 2>/dev/null | paste -sd ';' -; }
log_is() { [ "$(script_log)" = "$1" ] || { echo "    script-log: $(script_log)"; return 1; }; }

# A package that shares a directory and a file with "other".
make_shared() { # make_shared NAME
  _st=$TMP/stage-$1
  rm -rf "$_st"
  mkdir -p "$_st/usr/share/common" "$_st/usr/share/$1"
  echo "shared" > "$_st/usr/share/common/shared.txt"
  echo "$1" > "$_st/usr/share/$1/own.txt"
  pack "$_st" "$1-1.0-noarch-1.tgz"
}

# Give an installed package an uninstall script the way installpkg stores
# it: in douninst.sh/, and listed in the package's file list. The script
# appends to script-log when it runs.
put_pkgtools_douninst() { # put_pkgtools_douninst PKGNAME
  mkdir -p "$ROOT/var/lib/pkgtools/douninst.sh"
  echo "echo \"douninst $1\" >> script-log" \
    > "$ROOT/var/lib/pkgtools/douninst.sh/$1"
  chmod 755 "$ROOT/var/lib/pkgtools/douninst.sh/$1"
  echo "var/lib/pkgtools/douninst.sh/$1" >> "$ROOT/var/lib/pkgtools/packages/$1"
}
douninst_stored() { is_file "var/lib/pkgtools/douninst.sh/$1"; }

make_hello 1
make_hello 2

# ---------------------------------------------------------------------
# install
# ---------------------------------------------------------------------

begin "install"
new_root install
spkg -i "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "package is in the database" db_has hello-1-noarch-1
check "file list in the database" db_lists hello-1-noarch-1 usr/bin/hello
check "dirs in the database end with /" db_lists hello-1-noarch-1 usr/share/hello/
check "description in the database" db_lists hello-1-noarch-1 "hello: A package used by the spkg test suite."
check "full doinst.sh stored in scripts/" grep -q "ln -sf libhello.so.1 libhello.so" "$ROOT/var/lib/pkgtools/scripts/hello-1-noarch-1"
check "files installed" is_file usr/bin/hello
check "file content" content_is usr/bin/hello "hello 1"
check "file mode preserved" mode_is usr/bin/hello 755
check "nested file installed" is_file usr/share/hello/v1.txt
check "doinst.sh ran with -install" content_is hello-doinst-ran "doinst -install 1"
check "symlink from doinst.sh created" link_to usr/lib/libhello.so libhello.so.1
check "config file moved into place" content_is etc/hello.conf "setting=1"
check "config .new file consumed" absent etc/hello.conf.new
check "install/ removed" absent install
check "database lock created" [ -e "$ROOT/var/lib/pkgtools/packages/.lock" ]

begin "install: package database layout is created"
for d in var/lib/pkgtools/packages var/lib/pkgtools/scripts var/lib/pkgtools/setup \
         var/log/pkgtools/removed_packages var/log/pkgtools/removed_scripts \
         var/log/pkgtools/removed_uninstall_scripts; do
  check "$d exists" is_dir "$d"
done
check "var/log/packages symlink" link_to var/log/packages ../lib/pkgtools/packages
check "var/log/scripts symlink" link_to var/log/scripts ../lib/pkgtools/scripts
check "var/log/setup symlink" link_to var/log/setup ../lib/pkgtools/setup
check "var/log/removed_packages symlink" link_to var/log/removed_packages pkgtools/removed_packages
check "var/log/removed_scripts symlink" link_to var/log/removed_scripts pkgtools/removed_scripts
check "var/log/removed_uninstall_scripts symlink" \
  link_to var/log/removed_uninstall_scripts pkgtools/removed_uninstall_scripts
check "symlinks resolve" [ -d "$ROOT/var/log/packages" ] && [ -d "$ROOT/var/log/removed_uninstall_scripts" ]

begin "install: same package again is skipped"
spkg -i "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "reports skip" output_has "Skipping package hello-1-noarch-1"

begin "install: other version with same name is skipped"
spkg -i "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "reports skip" output_has "Skipping package hello-2-noarch-1"
check "new version not installed" db_lacks hello-2-noarch-1
check "files untouched" content_is usr/bin/hello "hello 1"

begin "install: existing var/log directory is left alone"
new_root compat-dirs
mkdir -p "$ROOT/var/log/packages"
spkg -i "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "directory kept" is_dir var/log/packages
check "other symlinks created" link_to var/log/scripts ../lib/pkgtools/scripts

# --force installs a second version next to the first one. Both are in the
# database and own the files they have in common, like installpkg does when
# it's given a package whose base name is already installed.
begin "install: --force installs another version next to the installed one"
new_root force
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -i -f "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "old version still in the database" db_has hello-1-noarch-1
check "new version in the database" db_has hello-2-noarch-1
check "shared file has the new content" content_is usr/bin/hello "hello 2"
check "file of the old version kept" content_is usr/share/hello/v1.txt "data 1"
check "file of the new version" content_is usr/share/hello/v2.txt "data 2"
check "both own the shared file" sh -c \
  'grep -qx usr/bin/hello "$1/hello-1-noarch-1" && grep -qx usr/bin/hello "$1/hello-2-noarch-1"' \
  - "$ROOT/var/lib/pkgtools/packages"
spkg -l hello
check "list exit status 0" rc_is 0
check "list shows the old version" output_has "^hello-1-noarch-1$"
check "list shows the new version" output_has "^hello-2-noarch-1$"

begin "install: --force with the same version is still skipped"
spkg -i -f "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "reports skip" output_has "Skipping package hello-2-noarch-1"

begin "remove: one of two versions by full name"
spkg -d hello-1-noarch-1
check "exit status 0" rc_is 0
check "removed from the database" db_lacks hello-1-noarch-1
check "other version still in the database" db_has hello-2-noarch-1
check "shared file kept" content_is usr/bin/hello "hello 2"
check "file only the removed version had is gone" absent usr/share/hello/v1.txt
check "file of the other version kept" content_is usr/share/hello/v2.txt "data 2"

# Which version a short name picks when several are installed is not
# checked: spkg takes the first one the directory listing returns, while
# removepkg takes the most recently installed one.
begin "remove: by short name with two versions installed removes one"
new_root force-remove
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -i -f "$PKGS/hello-2-noarch-1.tgz"
spkg -d hello
check "exit status 0" rc_is 0
check "exactly one version left" [ "$(ls "$ROOT/var/lib/pkgtools/packages" | wc -l)" = 1 ]
spkg -d hello
check "second remove exit status 0" rc_is 0
check "database empty" [ -z "$(ls "$ROOT/var/lib/pkgtools/packages")" ]
check "all files gone" absent usr/bin/hello

# upgradepkg removes every other installed version of the package.
make_hello 3
begin "upgrade: with two versions installed"
new_root force-upgrade
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -i -f "$PKGS/hello-2-noarch-1.tgz"
spkg -u "$PKGS/hello-3-noarch-1.tgz"
check "exit status 0" rc_is 0
check "new version in the database" db_has hello-3-noarch-1
check "file content" content_is usr/bin/hello "hello 3"
xfail "no other version left in the database" \
  [ "$(ls "$ROOT/var/lib/pkgtools/packages")" = hello-3-noarch-1 ]
xfail "files of the old versions removed" \
  sh -c '! [ -e "$1/usr/share/hello/v1.txt" ] && ! [ -e "$1/usr/share/hello/v2.txt" ]' - "$ROOT"

for ext in txz tlz tbr tar; do
  case $ext in
    txz) tool=xzdec ;;
    tlz) tool=lzma ;;
    tbr) tool=brotli ;;
    tar) tool=tar ;;
  esac
  begin "install: .$ext package"
  if ! command -v $tool >/dev/null 2>&1; then
    echo "    skipped, $tool not found"
    continue
  fi
  make_hello 1 $ext
  new_root fmt-$ext
  spkg -i "$PKGS/hello-1-noarch-1.$ext"
  check "exit status 0" rc_is 0
  check "package is in the database" db_has hello-1-noarch-1
  check "file content" content_is usr/bin/hello "hello 1"
done

begin "install: hardlinks"
st=$TMP/stage-links
mkdir -p "$st/usr/share/links"
echo data > "$st/usr/share/links/a"
ln "$st/usr/share/links/a" "$st/usr/share/links/b"
pack "$st" links-1.0-noarch-1.tgz
new_root links
spkg -i "$PKGS/links-1.0-noarch-1.tgz"
check "exit status 0" rc_is 0
check "both names exist" is_file usr/share/links/b
check "same inode" [ "$ROOT/usr/share/links/a" -ef "$ROOT/usr/share/links/b" ]
check "content" content_is usr/share/links/b data

begin "install: --no-scripts"
new_root no-scripts
spkg -i --no-scripts "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "doinst.sh not run" absent hello-doinst-ran
check "config file left as .new" is_file etc/hello.conf.new
check "symlink still created (extracted from doinst.sh)" link_to usr/lib/libhello.so libhello.so.1
check "install/ removed" absent install

begin "install: --no-fast-symlinks"
new_root no-fast
spkg -i --no-fast-symlinks "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "doinst.sh ran" content_is hello-doinst-ran "doinst -install 1"
check "symlink created by doinst.sh" link_to usr/lib/libhello.so libhello.so.1

begin "install: --no-fast-symlinks --no-scripts"
new_root no-fast-no-scripts
spkg -i --no-fast-symlinks --no-scripts "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "symlink not created" absent usr/lib/libhello.so

begin "install: existing file is replaced"
new_root overwrite
mkdir -p "$ROOT/usr/bin"
echo local > "$ROOT/usr/bin/hello"
spkg -i "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "warns" output_has "File already exist usr/bin/hello"
check "file replaced" content_is usr/bin/hello "hello 1"
check "no temporary file left" absent "usr/bin/hello--###install###"

begin "install: --safe refuses to replace an existing file"
new_root safe
mkdir -p "$ROOT/usr/bin"
echo local > "$ROOT/usr/bin/hello"
spkg -i --safe "$PKGS/hello-1-noarch-1.tgz"
check "exit status 2" rc_is 2
check "reports the file" output_has "File already exist. (usr/bin/hello)"
check "existing file untouched" content_is usr/bin/hello local
check "package not in the database" db_lacks hello-1-noarch-1
check "rolled back: other files not installed" absent usr/lib/libhello.so.1
check "rolled back: new directories removed" absent usr/share/hello
check "rolled back: no temporary files" [ -z "$(find "$ROOT" -name '*--###*')" ]

begin "install: --safe doesn't run doinst.sh"
new_root safe-clean
spkg -i --safe "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "doinst.sh not run" absent hello-doinst-ran
check "symlink still created" link_to usr/lib/libhello.so libhello.so.1
check "install/ removed" absent install

begin "install: --dry-run changes nothing"
new_root dry-install
spkg -i "$PKGS/hello-1-noarch-1.tgz"   # creates the database dirs
spkg -d hello
before=$(snapshot)
spkg -i -n "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "tree unchanged" [ "$before" = "$(snapshot)" ]

begin "install: --dry-run on a root without a package database"
new_root dry-no-db
spkg -i -n "$PKGS/hello-1-noarch-1.tgz"
check "exit status 2" rc_is 2
check "says what's missing" output_has "There is no package database in $ROOT/"
check "explains" output_has "With --dry-run, spkg doesn't create the package database"
check "root left empty" [ -z "$(ls -A "$ROOT")" ]

begin "install: --dry-run on a database with only packages/"
new_root dry-partial-db
make_shared one
spkg -i "$PKGS/one-1.0-noarch-1.tgz"
rm -rf "$ROOT/var/lib/pkgtools/setup" "$ROOT/var/log/pkgtools"
before=$(snapshot)
spkg -i -n "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "tree unchanged" [ "$before" = "$(snapshot)" ]

begin "install: invalid package name"
new_root bad-name
cp "$PKGS/hello-1-noarch-1.tgz" "$PKGS/hello.tgz"
spkg -i "$PKGS/hello.tgz"
check "exit status 2" rc_is 2
check "reports invalid name" output_has "Package name is invalid"

begin "install: missing package file"
spkg -i "$PKGS/missing-1.0-noarch-1.tgz"
check "exit status 2" rc_is 2
check "reports missing file" output_has "Package file does not exist"

begin "install: no packages given"
spkg -i
check "exit status 1" rc_is 1
check "reports usage error" output_has "No packages specified"

begin "install: requires root unless --dry-run"
if [ "$SPKG_REAL_UID" != 0 ]; then
  new_root not-root
  env -u LD_PRELOAD -u FAKEROOTKEY "$SPKG" --root="$ROOT" -i "$PKGS/hello-1-noarch-1.tgz" >"$OUT" 2>&1
  rc=$?
  check "exit status 1" rc_is 1
  check "reports missing privileges" output_has "You need root privileges"
  check "nothing installed" absent usr
else
  echo "    skipped, running as root"
fi

# ---------------------------------------------------------------------
# install: file metadata
# ---------------------------------------------------------------------
#
# Owners, modes (with setuid/setgid/sticky bits), mtimes, fifos and device
# nodes come from the archive. The stage directory is
# made under fakeroot too, so tar records the fake owners and devices.

# make_meta VERSION: version 2 changes the owner and mode of "owned"
make_meta() {
  _v=$1 _st=$TMP/stage-meta-$1
  rm -rf "$_st"
  mkdir -p "$_st/usr/bin" "$_st/usr/share/meta/private" "$_st/usr/share/meta/shared" \
           "$_st/dev"
  echo "suid" > "$_st/usr/bin/suid"; chmod 4755 "$_st/usr/bin/suid"
  echo "sgid" > "$_st/usr/bin/sgid"; chgrp 5678 "$_st/usr/bin/sgid"; chmod 2755 "$_st/usr/bin/sgid"
  echo "owned $_v" > "$_st/usr/share/meta/owned"
  if [ "$_v" = 1 ]; then
    chown 1234:5678 "$_st/usr/share/meta/owned"; chmod 640 "$_st/usr/share/meta/owned"
  else
    chown 4321:8765 "$_st/usr/share/meta/owned"; chmod 600 "$_st/usr/share/meta/owned"
  fi
  echo "old" > "$_st/usr/share/meta/old"
  touch -d "2001-02-03 04:05:06 UTC" "$_st/usr/share/meta/old"
  chmod 700 "$_st/usr/share/meta/private"
  chown 1234:5678 "$_st/usr/share/meta/shared"; chmod 3775 "$_st/usr/share/meta/shared"
  mkfifo -m 600 "$_st/dev/fifo"
  mknod -m 620 "$_st/dev/chr" c 4 64
  mknod -m 660 "$_st/dev/blk" b 8 1
  mknod -m 600 "$_st/dev/big" c 259 300
  (cd "$_st" && tar --numeric-owner --transform 's,^\./\(.\),\1,S' -cf - .) \
    | gzip > "$PKGS/meta-$_v-noarch-1.tgz"
}
make_meta 1
make_meta 2

# meta_is PATH FORMAT EXPECTED: stat -c FORMAT of PATH (not following symlinks)
meta_is() {
  _got=$(LC_ALL=C stat -c "$2" "$ROOT/$1" 2>&1)
  [ "$_got" = "$3" ] || { echo "    $1: '$_got', want '$3'"; return 1; }
}

begin "install: file metadata"
new_root meta
spkg -i "$PKGS/meta-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "setuid file" meta_is usr/bin/suid "%a %u %g" "4755 0 0"
check "setgid file" meta_is usr/bin/sgid "%a %u %g" "2755 0 5678"
check "owner and mode" meta_is usr/share/meta/owned "%a %u %g" "640 1234 5678"
check "mtime" meta_is usr/share/meta/old "%Y" "$(date -d '2001-02-03 04:05:06 UTC' +%s)"
check "private directory" meta_is usr/share/meta/private "%a %F" "700 directory"
check "setgid sticky directory" meta_is usr/share/meta/shared "%a %u %g" "3775 1234 5678"
check "fifo" meta_is dev/fifo "%F %a" "fifo 600"
check "character device" meta_is dev/chr "%F %a %Hr %Lr" "character special file 620 4 64"
check "block device" meta_is dev/blk "%F %a %Hr %Lr" "block special file 660 8 1"
check "device with numbers above 255" meta_is dev/big "%F %Hr %Lr" "character special file 259 300"
check "device nodes in the file list" db_lists meta-1-noarch-1 dev/chr

begin "upgrade: file metadata changes"
spkg -u "$PKGS/meta-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "new owner and mode" meta_is usr/share/meta/owned "%a %u %g" "600 4321 8765"
check "new content" content_is usr/share/meta/owned "owned 2"
check "setuid kept" meta_is usr/bin/suid "%a %u %g" "4755 0 0"

# ---------------------------------------------------------------------
# install: broken and malicious packages
# ---------------------------------------------------------------------

begin "install: truncated package is rolled back"
new_root truncated
size=$(stat -c %s "$PKGS/hello-1-noarch-1.tgz")
head -c $((size * 2 / 3)) "$PKGS/hello-1-noarch-1.tgz" > "$PKGS/trunc-1.0-noarch-1.tgz"
spkg -i "$PKGS/trunc-1.0-noarch-1.tgz"
check "exit status 2" rc_is 2
check "package not in the database" db_lacks trunc-1.0-noarch-1
check "no files left behind" absent usr
check "no temporary files" [ -z "$(find "$ROOT" -name '*--###*')" ]
# the stripped doinst.sh is written outside of the transaction
check "rolled back: install/ removed" absent install

begin "install: package truncated in the middle of a file"
st=$TMP/stage-bigfile
mkdir -p "$st/usr/share/big"
echo small > "$st/usr/share/big/a"
head -c 3000000 /dev/urandom > "$st/usr/share/big/blob"
for v in 1 2; do
  pack "$st" bigfile-$v-noarch-1.tar ./ usr/ usr/share/ usr/share/big/ \
    usr/share/big/a usr/share/big/blob
done
size=$(stat -c %s "$PKGS/bigfile-2-noarch-1.tar")
head -c $((size / 2)) "$PKGS/bigfile-2-noarch-1.tar" > "$PKGS/bigtrunc-2-noarch-1.tar"
new_root bigtrunc
spkg -i "$PKGS/bigtrunc-2-noarch-1.tar"
check "exit status 2" rc_is 2
check "no partial file left" absent usr/share/big/blob
check "everything rolled back" absent usr
check "package not in the database" db_lacks bigtrunc-2-noarch-1

begin "install: truncated file replacing an existing one"
new_root bigtrunc-replace
mkdir -p "$ROOT/usr/share/big"
echo local > "$ROOT/usr/share/big/blob"
spkg -i "$PKGS/bigtrunc-2-noarch-1.tar"
check "exit status 2" rc_is 2
check "existing file untouched" content_is usr/share/big/blob local
check "no temporary files" [ -z "$(find "$ROOT" -name '*--###*')" ]
check "nothing else left" absent usr/share/big/a

begin "upgrade: package truncated in the middle of a file"
new_root bigtrunc-upgrade
spkg -i "$PKGS/bigfile-1-noarch-1.tar"
cp "$PKGS/bigtrunc-2-noarch-1.tar" "$PKGS/bigfile-2-noarch-1.tar"
before=$(snapshot)
spkg -u "$PKGS/bigfile-2-noarch-1.tar"
check "exit status 2" rc_is 2
check "old version still in the database" db_has bigfile-1-noarch-1
check "tree unchanged" [ "$before" = "$(snapshot)" ]

begin "install: garbage package"
new_root garbage
printf 'this is not a tar archive\n%.0s' $(seq 100) | gzip > "$PKGS/garbage-1.0-noarch-1.tgz"
spkg -i "$PKGS/garbage-1.0-noarch-1.tgz"
check "exit status 2" rc_is 2
check "package not in the database" db_lacks garbage-1.0-noarch-1

begin "install: path with ../ is rejected"
st=$TMP/stage-evil/x
mkdir -p "$st/usr" "$TMP/stage-evil/escaped"
echo ok > "$st/usr/ok"
echo evil > "$TMP/stage-evil/escaped/evil"
pack "$st" evil-1.0-noarch-1.tgz ./usr/ ./usr/ok ../escaped/evil
new_root evil
spkg -i "$PKGS/evil-1.0-noarch-1.tgz"
check "exit status 2" rc_is 2
check "reports unsafe path" output_has "unsafe path"
check "nothing written outside the root" [ ! -e "$TMP/escaped" ]
check "rolled back: earlier file removed" absent usr/ok
check "package not in the database" db_lacks evil-1.0-noarch-1

begin "install: absolute path is rejected"
abs=$TMP/abs-target
echo evil > "$abs"
pack "$st" abs-1.0-noarch-1.tgz ./usr/ ./usr/ok "$abs"
rm -f "$abs"
new_root abs
spkg -i "$PKGS/abs-1.0-noarch-1.tgz"
check "exit status 2" rc_is 2
check "reports unsafe path" output_has "unsafe path"
check "absolute path not written" [ ! -e "$abs" ]
check "rolled back: earlier file removed" absent usr/ok

begin "install: unsafe symlink in doinst.sh is rejected"
st=$TMP/stage-evil-link
mkdir -p "$st/install" "$st/usr"
echo ok > "$st/usr/ok"
echo '( cd ../.. ; ln -sf /etc/passwd evil )' > "$st/install/doinst.sh"
pack "$st" evillink-1.0-noarch-1.tgz
new_root evillink
spkg -i "$PKGS/evillink-1.0-noarch-1.tgz"
check "exit status 2" rc_is 2
check "reports unsafe symlink" output_has "symlink with unsafe path"
check "no symlink outside the root" [ ! -L "$(dirname "$TMP")/evil" ]
check "package not in the database" db_lacks evillink-1.0-noarch-1

begin "install: symlink inside the archive"
st=$TMP/stage-symlink
mkdir -p "$st/usr/bin"
echo bin > "$st/usr/bin/real"
ln -s real "$st/usr/bin/link"
pack "$st" symlink-1.0-noarch-1.tgz ./usr/ ./usr/bin/ ./usr/bin/real ./usr/bin/link
new_root symlink
spkg -i "$PKGS/symlink-1.0-noarch-1.tgz"
if [ "${SPKG_LEGACY_CHECKS:-0}" = 1 ]; then
  # default build: makepkg turns symlinks into doinst.sh lines, so a
  # symlink in the archive means a broken package
  check "exit status 2" rc_is 2
  check "reports symlink" output_has "Symlink was found in the archive"
  check "rolled back: earlier file removed" absent usr/bin/real
  check "package not in the database" db_lacks symlink-1.0-noarch-1
else
  check "exit status 0" rc_is 0
  check "symlink installed" link_to usr/bin/link real
fi

begin "install: metadata of a symlink inside the archive"
if [ "${SPKG_LEGACY_CHECKS:-0}" = 1 ]; then
  echo "    skipped, symlinks in the archive are rejected"
else
  st=$TMP/stage-symlink-meta
  rm -rf "$st"; mkdir -p "$st/usr/bin"
  echo bin > "$st/usr/bin/real"
  chown 1234:5678 "$st/usr/bin/real"; chmod 640 "$st/usr/bin/real"
  ln -s real "$st/usr/bin/link"
  chown -h 4321:8765 "$st/usr/bin/link"
  touch -h -d "2002-03-04 05:06:07 UTC" "$st/usr/bin/link"
  (cd "$st" && tar --numeric-owner --no-recursion -cf - ./usr/ ./usr/bin/ ./usr/bin/real ./usr/bin/link) \
    | gzip > "$PKGS/symlink-meta-1.0-noarch-1.tgz"
  new_root symlink-meta
  spkg -i "$PKGS/symlink-meta-1.0-noarch-1.tgz"
  check "exit status 0" rc_is 0
  # the symlink is created on finalize, without the owner and mtime from
  # the archive (tar -xp in installpkg sets them)
  xfail "symlink owner" meta_is usr/bin/link "%u %g" "4321 8765"
  xfail "symlink mtime" meta_is usr/bin/link "%Y" "$(date -d '2002-03-04 05:06:07 UTC' +%s)"
  check "target untouched" meta_is usr/bin/real "%a %u %g" "640 1234 5678"
fi

# ---------------------------------------------------------------------
# database lock
# ---------------------------------------------------------------------
#
# Commands that change the database take the lock on packages/.lock and
# give up after about 2 seconds. -l and --dry-run don't take it.

if ! command -v flock >/dev/null 2>&1; then
  begin "lock"
  echo "    skipped, flock not found"
else
  # hold_lock: take the lock in the background until release_lock
  hold_lock() {
    rm -f "$TMP/lock-held"; touch "$TMP/lock-hold"
    ( exec 9>"$ROOT/var/lib/pkgtools/packages/.lock"
      flock 9 && touch "$TMP/lock-held"
      while [ -e "$TMP/lock-hold" ]; do sleep 0.1; done ) &
    lock_pid=$!
    _n=0
    while [ ! -e "$TMP/lock-held" ] && [ $_n -lt 100 ]; do sleep 0.1; _n=$((_n + 1)); done
  }
  release_lock() { rm -f "$TMP/lock-hold"; wait $lock_pid; }
  lock_free() { flock -n "$ROOT/var/lib/pkgtools/packages/.lock" true; }

  begin "lock: install while the database is locked"
  new_root lock
  spkg -i "$PKGS/hello-1-noarch-1.tgz"
  check "lock released after the install" lock_free
  hold_lock
  before=$(snapshot)
  spkg -i "$PKGS/links-1.0-noarch-1.tgz"
  check "exit status 2" rc_is 2
  check "reports the lock" output_has "database is already locked"
  check "tree unchanged" [ "$before" = "$(snapshot)" ]

  begin "lock: upgrade and remove while the database is locked"
  spkg -u "$PKGS/hello-2-noarch-1.tgz"
  check "upgrade exit status 2" rc_is 2
  spkg -d hello
  check "remove exit status 2" rc_is 2
  check "tree unchanged" [ "$before" = "$(snapshot)" ]

  begin "lock: list and --dry-run don't need the lock"
  spkg -l
  check "list as root exit status 0" rc_is 0
  check "list as root output" output_has "^hello-1-noarch-1$"
  if [ "$SPKG_REAL_UID" != 0 ]; then
    env -u LD_PRELOAD -u FAKEROOTKEY "$SPKG" --root="$ROOT" -l >"$OUT" 2>&1
    rc=$?
    check "list as a user exit status 0" rc_is 0
    check "list as a user output" output_has "^hello-1-noarch-1$"
  fi
  spkg -u -n "$PKGS/hello-2-noarch-1.tgz"
  check "dry-run exit status 0" rc_is 0
  check "tree unchanged" [ "$before" = "$(snapshot)" ]
  release_lock

  begin "lock: waits for a lock that is released soon"
  hold_lock
  ( sleep 0.5; rm -f "$TMP/lock-hold" ) &
  spkg -i "$PKGS/links-1.0-noarch-1.tgz"
  wait
  check "exit status 0" rc_is 0
  check "installed" db_has links-1.0-noarch-1
  check "lock released" lock_free
fi

# ---------------------------------------------------------------------
# list
# ---------------------------------------------------------------------

begin "list"
new_root list
spkg -i "$PKGS/hello-1-noarch-1.tgz"
make_shared other
spkg -i "$PKGS/other-1.0-noarch-1.tgz"
spkg -l
check "exit status 0" rc_is 0
check "lists hello" output_has "hello-1-noarch-1"
check "lists other" output_has "other-1.0-noarch-1"
spkg -l 'oth*'
check "glob: exit status 0" rc_is 0
check "glob: lists other" output_has "other-1.0-noarch-1"
check "glob: hello filtered out" [ -z "$(grep hello "$OUT")" ]

begin "list: root without a package database"
new_root list-nodb
spkg -l
check "exit status 2" rc_is 2
check "explains" output_has "There is no package database"
check "database not created" absent var

# ---------------------------------------------------------------------
# remove
# ---------------------------------------------------------------------

begin "remove"
new_root remove
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -d hello
check "exit status 0" rc_is 0
check "package removed from the database" db_lacks hello-1-noarch-1
check "entry moved to removed_packages" removed_entry hello-1-noarch-1
check "files removed" absent usr/bin/hello
check "symlink removed" absent usr/lib/libhello.so
check "empty directories removed" absent usr
check "config file kept" content_is etc/hello.conf "setting=1"

begin "remove: by full package name"
new_root remove-full
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -d hello-1-noarch-1
check "exit status 0" rc_is 0
check "package removed" db_lacks hello-1-noarch-1

begin "remove: --purge removes the config file"
new_root purge
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -d --purge hello
check "exit status 0" rc_is 0
check "config file removed" absent etc/hello.conf

begin "remove: shared files and directories are kept"
new_root shared
make_shared one
make_shared two
spkg -i "$PKGS/one-1.0-noarch-1.tgz"
spkg -i "$PKGS/two-1.0-noarch-1.tgz"
spkg -d one
check "exit status 0" rc_is 0
check "own file removed" absent usr/share/one
check "shared file kept" is_file usr/share/common/shared.txt
check "other package untouched" is_file usr/share/two/own.txt
spkg -d two
check "last owner removed: shared file removed" absent usr/share/common
check "all directories removed" absent usr

begin "remove: changed file"
new_root changed
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -i "$PKGS/one-1.0-noarch-1.tgz"
touch -d "+1 hour" "$ROOT/usr/bin/hello" "$ROOT/usr/share/one/own.txt"
spkg -d hello
check "exit status 0" rc_is 0
check "warns" output_has "File was changed after installation. (usr/bin/hello)"
check "removed without --safe" absent usr/bin/hello
spkg -d --safe one
check "--safe: exit status 0" rc_is 0
check "--safe: changed file kept" is_file usr/share/one/own.txt
check "--safe: package removed from the database" db_lacks one-1.0-noarch-1

begin "remove: package not installed"
new_root remove-missing
spkg -d nothere
check "exit status 2" rc_is 2
check "reports missing package" output_has "Package not found"

begin "remove: --dry-run changes nothing"
new_root dry-remove
spkg -i "$PKGS/hello-1-noarch-1.tgz"
before=$(snapshot)
spkg -d -n hello
check "exit status 0" rc_is 0
check "tree unchanged" [ "$before" = "$(snapshot)" ]

begin "install: douninst.sh is stored like installpkg does"
new_root douninst-install
spkg -i "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "stored under the full package name" douninst_stored hello-1-noarch-1
check "same content" cmp -s "$TMP/stage-hello-1/install/douninst.sh" \
  "$ROOT/var/lib/pkgtools/douninst.sh/hello-1-noarch-1"
check "mode 755" mode_is var/lib/pkgtools/douninst.sh/hello-1-noarch-1 755
check "listed in the file list" \
  db_lists hello-1-noarch-1 var/lib/pkgtools/douninst.sh/hello-1-noarch-1
check "install/douninst.sh listed too" db_lists hello-1-noarch-1 install/douninst.sh
check "not run at install" log_is "doinst 1"
check "not left in install/" absent install

begin "install: --dry-run doesn't store douninst.sh"
new_root douninst-install-dry
spkg -i "$PKGS/one-1.0-noarch-1.tgz" # dry-run needs an existing database
spkg -i -n "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "not stored" absent var/lib/pkgtools/douninst.sh

begin "install: failed install doesn't leave douninst.sh behind"
st=$TMP/stage-hello-1
pack "$st" brokenuninst-1-noarch-1.tgz ./ install/ install/douninst.sh \
  usr/ usr/bin/ usr/bin/hello ../stage-hello-2/usr/bin/hello
new_root douninst-rollback
spkg -i "$PKGS/brokenuninst-1-noarch-1.tgz"
check "exit status 2" rc_is 2
check "not stored" absent var/lib/pkgtools/douninst.sh/brokenuninst-1-noarch-1
check "no temporary files" [ -z "$(find "$ROOT" -name '*--###*')" ]

begin "remove: douninst.sh"
new_root douninst
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -d hello
check "exit status 0" rc_is 0
check "ran after the files were removed" log_is "doinst 1;douninst 1 saw "
check "script moved to removed_uninstall_scripts" \
  is_file var/log/pkgtools/removed_uninstall_scripts/hello-1-noarch-1
check "stored script removed" absent var/lib/pkgtools/douninst.sh/hello-1-noarch-1
check "no copy left behind" absent var/lib/pkgtools/setup/tmp/spkg/hello-1-noarch-1
check "removed script has mode 755" \
  mode_is var/log/pkgtools/removed_uninstall_scripts/hello-1-noarch-1 755

# var/tmp is writable by everyone, so a user could create var/tmp/spkg
# before spkg does and put a symlink where the copy of douninst.sh goes.
# spkg must neither write through it nor run what it points to.
plant_douninst_symlink() { # plant_douninst_symlink PKGNAME
  echo "precious" > "$ROOT/victim"
  mkdir -p "$ROOT/var/tmp/spkg"
  chmod 1777 "$ROOT/var/tmp" "$ROOT/var/tmp/spkg"
  ln -sf "$ROOT/victim" "$ROOT/var/tmp/spkg/$1"
}

begin "remove: douninst.sh copy can't be redirected through var/tmp"
new_root douninst-symlink
spkg -i "$PKGS/hello-1-noarch-1.tgz"
plant_douninst_symlink hello-1-noarch-1
spkg -d hello
check "exit status 0" rc_is 0
check "symlink target not written" content_is victim "precious"
check "the package's own script ran" log_is "doinst 1;douninst 1 saw "
check "removed script is the package's" \
  content_is var/log/pkgtools/removed_uninstall_scripts/hello-1-noarch-1 \
  'echo "douninst 1 saw $(cat usr/bin/hello 2>/dev/null)" >> script-log'

begin "remove: douninst.sh with the current directory as root"
new_root douninst-dot
spkg -i "$PKGS/hello-1-noarch-1.tgz"
(cd "$ROOT" && "$SPKG" --root=. -d hello) >"$OUT" 2>&1
rc=$?
check "exit status 0" rc_is 0
check "douninst.sh ran" log_is "doinst 1;douninst 1 saw "

begin "upgrade: douninst.sh copy can't be redirected through var/tmp"
new_root upgrade-douninst-symlink
spkg -i "$PKGS/hello-1-noarch-1.tgz"
plant_douninst_symlink hello-1-noarch-1
spkg -u "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "symlink target not written" content_is victim "precious"
check "the old package's script ran" log_is "doinst 1;douninst 1 saw hello 2;doinst 2"
check "removed script has mode 755" \
  mode_is var/log/pkgtools/removed_uninstall_scripts/hello-1-noarch-1 755

begin "remove: douninst.sh stored by installpkg"
new_root douninst-pkgtools
make_shared one
spkg -i "$PKGS/one-1.0-noarch-1.tgz"
put_pkgtools_douninst one-1.0-noarch-1
spkg -d one
check "exit status 0" rc_is 0
check "douninst.sh ran" log_is "douninst one-1.0-noarch-1"
check "stored script removed" absent var/lib/pkgtools/douninst.sh/one-1.0-noarch-1

begin "remove: failing douninst.sh"
new_root douninst-fail
spkg -i "$PKGS/one-1.0-noarch-1.tgz"
put_pkgtools_douninst one-1.0-noarch-1
echo "exit 3" >> "$ROOT/var/lib/pkgtools/douninst.sh/one-1.0-noarch-1"
spkg -d one
check "exit status 0" rc_is 0
check "warns" output_has "douninst.sh script execution failed"
check "package removed" db_lacks one-1.0-noarch-1
check "script moved to removed_uninstall_scripts anyway (like removepkg)" \
  is_file var/log/pkgtools/removed_uninstall_scripts/one-1.0-noarch-1

begin "remove: --no-scripts skips douninst.sh"
new_root douninst-no-scripts
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -d --no-scripts hello
check "exit status 0" rc_is 0
check "douninst.sh not run" log_is "doinst 1"
check "stored script removed with the package" \
  absent var/lib/pkgtools/douninst.sh/hello-1-noarch-1

begin "remove: --dry-run doesn't run douninst.sh"
new_root douninst-dry
spkg -i "$PKGS/hello-1-noarch-1.tgz"
before=$(snapshot)
spkg -d -n hello
check "exit status 0" rc_is 0
check "douninst.sh not run" log_is "doinst 1"
check "tree unchanged" [ "$before" = "$(snapshot)" ]

# ---------------------------------------------------------------------
# upgrade
# ---------------------------------------------------------------------

begin "upgrade"
new_root upgrade
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -u "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "new version in the database" db_has hello-2-noarch-1
check "old version gone from the database" db_lacks hello-1-noarch-1
check "old entry moved to removed_packages" removed_entry hello-1-noarch-1
check "file updated" content_is usr/bin/hello "hello 2"
check "file mode preserved" mode_is usr/bin/hello 755
check "new file installed" is_file usr/share/hello/v2.txt
check "leftover file removed" absent usr/share/hello/v1.txt
check "leftover library removed" absent usr/lib/libhello.so.1
check "symlink retargeted" link_to usr/lib/libhello.so libhello.so.2
check "doinst.sh of new version ran" content_is hello-doinst-ran "doinst -install 2"
check "changed config left as .new" content_is etc/hello.conf.new "setting=2"
check "existing config kept" content_is etc/hello.conf "setting=1"
check "no temporary files" [ -z "$(find "$ROOT" -name '*--###*')" ]

begin "upgrade: --no-scripts"
new_root upgrade-no-scripts
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -u --no-scripts "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "doinst.sh not run" content_is hello-doinst-ran "doinst -install 1"
check "install/ removed" absent install

begin "upgrade: shared file stays when the other owner drops it"
new_root upgrade-shared
make_shared one
spkg -i "$PKGS/one-1.0-noarch-1.tgz"
st=$TMP/stage-two-noshare
mkdir -p "$st/usr/share/common" "$st/usr/share/two"
echo two > "$st/usr/share/two/own.txt"
make_shared two
spkg -i "$PKGS/two-1.0-noarch-1.tgz"
pack "$st" two-2.0-noarch-1.tgz
spkg -u "$PKGS/two-2.0-noarch-1.tgz"
check "exit status 0" rc_is 0
check "shared file kept for the other package" is_file usr/share/common/shared.txt

begin "upgrade: package not installed is skipped"
new_root upgrade-missing
spkg -u "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "reports skip" output_has "NOT installed"
check "nothing installed" db_lacks hello-2-noarch-1

begin "upgrade: --install-new installs it"
spkg -u --install-new "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "installed" db_has hello-2-noarch-1

begin "upgrade: same version is skipped"
spkg -u "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "reports up to date" output_has "already uptodate"

begin "upgrade: --reinstall"
rm "$ROOT/usr/bin/hello"
spkg -u --reinstall "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "missing file restored" content_is usr/bin/hello "hello 2"
check "still in the database" db_has hello-2-noarch-1

begin "upgrade: implicit command (no -i/-u/-d) upgrades or installs"
new_root implicit
spkg "$PKGS/hello-1-noarch-1.tgz"
check "install: exit status 0" rc_is 0
check "installed" db_has hello-1-noarch-1
spkg "$PKGS/hello-2-noarch-1.tgz"
check "upgrade: exit status 0" rc_is 0
check "upgraded" db_has hello-2-noarch-1

begin "upgrade: truncated package is rolled back"
new_root upgrade-truncated
spkg -i "$PKGS/hello-1-noarch-1.tgz"
before=$(snapshot)
cp "$PKGS/trunc-1.0-noarch-1.tgz" "$PKGS/hello-3-noarch-1.tgz"
spkg -u "$PKGS/hello-3-noarch-1.tgz"
check "exit status 2" rc_is 2
check "old version still in the database" db_has hello-1-noarch-1
check "tree unchanged" [ "$before" = "$(snapshot)" ]
check "rolled back: install/ removed" absent install

begin "upgrade: --dry-run changes nothing"
new_root dry-upgrade
spkg -i "$PKGS/hello-1-noarch-1.tgz"
before=$(snapshot)
spkg -u -n "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "tree unchanged" [ "$before" = "$(snapshot)" ]

begin "upgrade: douninst.sh of the old version runs"
new_root upgrade-douninst
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -u "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
# like upgradepkg: the old script runs once the new version is in place,
# and before the new version's doinst.sh
check "old script ran between the two doinst.sh" \
  log_is "doinst 1;douninst 1 saw hello 2;doinst 2"
check "old script moved to removed_uninstall_scripts" \
  is_file var/log/pkgtools/removed_uninstall_scripts/hello-1-noarch-1
check "old stored script removed" absent var/lib/pkgtools/douninst.sh/hello-1-noarch-1
check "new script stored" douninst_stored hello-2-noarch-1
check "new script listed in the file list" \
  db_lists hello-2-noarch-1 var/lib/pkgtools/douninst.sh/hello-2-noarch-1
check "no copy left behind" absent var/lib/pkgtools/setup/tmp/spkg/hello-1-noarch-1
spkg -d hello
check "new script runs on removal" \
  log_is "doinst 1;douninst 1 saw hello 2;doinst 2;douninst 2 saw "

begin "upgrade: package installed by installpkg"
new_root upgrade-douninst-pkgtools
spkg -i "$PKGS/hello-1-noarch-1.tgz"
# make it look like installpkg installed it
rm "$ROOT/var/lib/pkgtools/douninst.sh/hello-1-noarch-1"
grep -v "^var/lib/pkgtools/douninst.sh/" "$ROOT/var/lib/pkgtools/packages/hello-1-noarch-1" \
  > "$TMP/entry" && cat "$TMP/entry" > "$ROOT/var/lib/pkgtools/packages/hello-1-noarch-1"
put_pkgtools_douninst hello-1-noarch-1
spkg -u "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "old script ran" log_is "doinst 1;douninst hello-1-noarch-1;doinst 2"
check "old stored script removed" absent var/lib/pkgtools/douninst.sh/hello-1-noarch-1
check "new script stored" douninst_stored hello-2-noarch-1

begin "upgrade: --reinstall"
new_root upgrade-douninst-reinstall
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -u --reinstall "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "old script ran" log_is "doinst 1;douninst 1 saw hello 1;doinst 1"
check "script still stored" douninst_stored hello-1-noarch-1

begin "upgrade: to a version without douninst.sh"
new_root upgrade-douninst-dropped
st=$TMP/stage-hello-3-nodouninst
rm -rf "$st"
cp -a "$TMP/stage-hello-2" "$st"
rm "$st/install/douninst.sh"
pack "$st" hello-3-noarch-1.tgz
spkg -i "$PKGS/hello-1-noarch-1.tgz"
spkg -u "$PKGS/hello-3-noarch-1.tgz"
check "exit status 0" rc_is 0
check "old script ran" log_is "doinst 1;douninst 1 saw hello 2;doinst 2"
check "nothing stored" [ -z "$(ls "$ROOT/var/lib/pkgtools/douninst.sh")" ]

begin "upgrade: failed upgrade keeps the old script"
new_root upgrade-douninst-rollback
spkg -i "$PKGS/hello-1-noarch-1.tgz"
before=$(snapshot)
cp "$PKGS/brokenuninst-1-noarch-1.tgz" "$PKGS/hello-4-noarch-1.tgz"
spkg -u "$PKGS/hello-4-noarch-1.tgz"
check "exit status 2" rc_is 2
check "old script not run" log_is "doinst 1"
check "old script still stored" douninst_stored hello-1-noarch-1
check "tree unchanged" [ "$before" = "$(snapshot)" ]

begin "upgrade: --dry-run doesn't run douninst.sh"
new_root upgrade-douninst-dry
spkg -i "$PKGS/hello-1-noarch-1.tgz"
before=$(snapshot)
spkg -u -n "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "douninst.sh not run" log_is "doinst 1"
check "tree unchanged" [ "$before" = "$(snapshot)" ]

# ---------------------------------------------------------------------
# output and helper programs
# ---------------------------------------------------------------------

# A root where installing hello-1 gives a warning (the existing file is
# replaced).
warn_root() {
  new_root output
  mkdir -p "$ROOT/usr/bin"; echo old > "$ROOT/usr/bin/hello"
}

begin "output: default shows info and warnings"
warn_root
spkg -i "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "info" output_has "^Installing package hello-1-noarch-1"
check "warning" output_has "^WARNING: File already exist usr/bin/hello"
check "no notices" sh -c '! grep -q "^ --> " "$1"' - "$OUT"

begin "output: -q hides warnings"
warn_root
spkg -i -q "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "info" output_has "^Installing package hello-1-noarch-1"
check "no warning" sh -c '! grep -q "^WARNING" "$1"' - "$OUT"

begin "output: -qq shows only errors"
warn_root
spkg -i -qq "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "no output" [ ! -s "$OUT" ]
spkg -i -qq "$TMP/nothere-1.0-noarch-1.tgz"
check "error exit status 2" rc_is 2
check "error shown" output_has "^ERROR: Package file does not exist"

begin "output: -v shows notices"
warn_root
spkg -i -v "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "files" output_has "^ --> Installing file usr/bin/hello"
check "script" output_has "^ --> Running post-installation script"
check "no debug messages" sh -c '! grep -q "Extracting symlinks" "$1"' - "$OUT"

begin "output: -vv shows debug messages"
warn_root
spkg -i -vv "$PKGS/hello-1-noarch-1.tgz"
check "exit status 0" rc_is 0
check "debug" output_has "^ --> Extracting symlinks from the post-installation script"

begin "output: -v and -q together"
warn_root
spkg -i -v -q "$PKGS/hello-1-noarch-1.tgz"
check "exit status 1" rc_is 1
check "reports the conflict" output_has "Verbose or quiet?"
check "nothing installed" db_lacks hello-1-noarch-1

# spkg runs /sbin/ldconfig -r ROOT when the package has a *.so file and
# ROOT has etc/ld.so.conf, and /usr/bin/gtk-update-icon-cache on ROOT's
# hicolor theme when the package has a *.desktop file. Both leave a file
# behind that shows they ran.
st=$TMP/stage-solib
rm -rf "$st"; mkdir -p "$st/usr/lib"
echo "lib" > "$st/usr/lib/libtest.so.1"; echo "lib" > "$st/usr/lib/libtest.so"
pack "$st" solib-1.0-noarch-1.tgz
st=$TMP/stage-desktop
rm -rf "$st"; mkdir -p "$st/usr/share/applications" "$st/usr/share/icons/hicolor/16x16/apps"
printf '[Desktop Entry]\nName=Test\nExec=true\nIcon=test\n' > "$st/usr/share/applications/test.desktop"
printf '[Icon Theme]\nName=Hicolor\nDirectories=16x16/apps\n\n[16x16/apps]\nSize=16\n' \
  > "$st/usr/share/icons/hicolor/index.theme"
: > "$st/usr/share/icons/hicolor/16x16/apps/test.png"
pack "$st" desktop-1.0-noarch-1.tgz
ld_cache() { is_file etc/ld.so.cache; }
ld_root() { new_root "$1"; mkdir -p "$ROOT/etc"; : > "$ROOT/etc/ld.so.conf"; }
icon_cache() { is_file usr/share/icons/hicolor/icon-theme.cache; }

if [ -x /sbin/ldconfig ]; then
  begin "install: runs ldconfig for a shared library"
  ld_root ldconfig
  spkg -i "$PKGS/solib-1.0-noarch-1.tgz"
  check "exit status 0" rc_is 0
  check "ldconfig ran" ld_cache

  begin "install: --no-ldconfig"
  ld_root no-ldconfig
  spkg -i --no-ldconfig "$PKGS/solib-1.0-noarch-1.tgz"
  check "exit status 0" rc_is 0
  check "ldconfig not run" absent etc/ld.so.cache

  begin "install: no ldconfig without a shared library"
  ld_root no-solib
  spkg -i "$PKGS/links-1.0-noarch-1.tgz"
  check "ldconfig not run" absent etc/ld.so.cache

  begin "upgrade: --no-ldconfig"
  spkg -i --no-ldconfig "$PKGS/solib-1.0-noarch-1.tgz"
  spkg -u --reinstall --no-ldconfig "$PKGS/solib-1.0-noarch-1.tgz"
  check "exit status 0" rc_is 0
  check "ldconfig not run" absent etc/ld.so.cache
  spkg -u --reinstall "$PKGS/solib-1.0-noarch-1.tgz"
  check "without it, ldconfig ran" ld_cache
else
  begin "ldconfig"
  echo "    skipped, /sbin/ldconfig not found"
fi

if [ -x /usr/bin/gtk-update-icon-cache ]; then
  begin "install: runs gtk-update-icon-cache for a .desktop file"
  new_root icons
  spkg -i "$PKGS/desktop-1.0-noarch-1.tgz"
  check "exit status 0" rc_is 0
  check "icon cache created" icon_cache

  begin "install: --no-gtk-update-icon-cache"
  new_root no-icons
  spkg -i --no-gtk-update-icon-cache "$PKGS/desktop-1.0-noarch-1.tgz"
  check "exit status 0" rc_is 0
  check "icon cache not created" absent usr/share/icons/hicolor/icon-theme.cache

  begin "upgrade: --no-gtk-update-icon-cache"
  spkg -u --reinstall --no-gtk-update-icon-cache "$PKGS/desktop-1.0-noarch-1.tgz"
  check "exit status 0" rc_is 0
  check "icon cache not created" absent usr/share/icons/hicolor/icon-theme.cache
  spkg -u --reinstall "$PKGS/desktop-1.0-noarch-1.tgz"
  check "without it, icon cache created" icon_cache
else
  begin "gtk-update-icon-cache"
  echo "    skipped, /usr/bin/gtk-update-icon-cache not found"
fi

# ---------------------------------------------------------------------
# several packages in one command
# ---------------------------------------------------------------------
#
# A package that fails doesn't stop the others: each one is installed,
# upgraded or removed on its own, and the exit status is 2 if any failed.

MULTI=$TMP/multi
mkdir -p "$MULTI"
# truncate PKGFILE OUTFILE: the first 2/3 of a package
truncate_pkg() { head -c $(($(stat -c %s "$1") * 2 / 3)) "$1" > "$2"; }
truncate_pkg "$PKGS/hello-1-noarch-1.tgz" "$MULTI/broken-1.0-noarch-1.tgz"
st=$TMP/stage-links-2
rm -rf "$st"; cp -a "$TMP/stage-links" "$st"
echo "data 2" > "$st/usr/share/links/c"
pack "$st" links-2.0-noarch-1.tgz
truncate_pkg "$PKGS/links-2.0-noarch-1.tgz" "$MULTI/links-2.0-noarch-1.tgz"

begin "install: several packages, one of them broken"
new_root multi-install
spkg -i "$PKGS/hello-1-noarch-1.tgz" "$MULTI/broken-1.0-noarch-1.tgz" \
  "$PKGS/links-1.0-noarch-1.tgz"
check "exit status 2" rc_is 2
check "first package installed" db_has hello-1-noarch-1
check "broken package not installed" db_lacks broken-1.0-noarch-1
check "package after the broken one installed" db_has links-1.0-noarch-1
check "files of the first package" content_is usr/bin/hello "hello 1"
check "files of the last package" content_is usr/share/links/a data
check "no temporary files" [ -z "$(find "$ROOT" -name '*--###*')" ]

begin "install: several packages, one missing"
new_root multi-missing
spkg -i "$MULTI/nothere-1.0-noarch-1.tgz" "$PKGS/hello-1-noarch-1.tgz"
check "exit status 2" rc_is 2
check "the other package installed" db_has hello-1-noarch-1

begin "upgrade: several packages, one of them broken"
new_root multi-upgrade
spkg -i "$PKGS/hello-1-noarch-1.tgz" "$PKGS/links-1.0-noarch-1.tgz"
spkg -u "$MULTI/links-2.0-noarch-1.tgz" "$PKGS/hello-2-noarch-1.tgz"
check "exit status 2" rc_is 2
check "broken upgrade rolled back" db_has links-1.0-noarch-1
check "broken package not in the database" db_lacks links-2.0-noarch-1
check "files of the broken upgrade untouched" absent usr/share/links/c
check "other package upgraded" db_has hello-2-noarch-1
check "other package's files" content_is usr/bin/hello "hello 2"

begin "remove: several packages, one not installed"
spkg -d hello nothere links
check "exit status 2" rc_is 2
check "reports missing package" output_has "Package not found"
check "first package removed" db_lacks hello-2-noarch-1
check "package after the missing one removed" db_lacks links-1.0-noarch-1
check "files removed" absent usr

# ---------------------------------------------------------------------
# compatibility with pkgtools
# ---------------------------------------------------------------------
#
# The same packages are installed, upgraded and removed with pkgtools in
# one root and with spkg (or a mix of both) in another, and the results are
# compared. Skipped when pkgtools isn't installed; PKGTOOLS=dir points to
# another copy.
#
# Known differences that aren't checked:
# - The package sizes in the database entry: installpkg writes du -h of
#   the package and the size of the tar stream, spkg whole kilobytes of the
#   package and of the files in it.
# - spkg runs doinst.sh with -install as $1, installpkg without arguments
#   (hello-doinst-ran records it).
# - installpkg creates var/lib/pkgtools/setup/tmp.
# - The names of the entries in var/log/pkgtools/removed_*: removepkg
#   keeps the package name, upgradepkg adds -upgraded-<time>, spkg adds
#   -removed-<time> to the package and script entries in both cases and
#   keeps the name of the uninstall script. They are compared without it.

PKGTOOLS=${PKGTOOLS:-/sbin}

# pt ROOTDIR TOOL ARGS... : run a pkgtools command on ROOTDIR
pt() {
  _r=$1 _t=$2; shift 2
  ROOT=$_r INSTLOCKDIR=$TMP/pkgtools-lock LC_ALL=C "$PKGTOOLS/$_t" "$@" >"$OUT" 2>&1
  rc=$?
  if [ "${VERBOSE:-0}" = 1 ]; then
    sed 's/^/    | /' "$OUT"
  fi
}

# Everything outside var/ (types, modes, owners, sizes, link targets and
# file contents), and the names in the package database with the
# timestamps of removed entries taken out.
pt_tree() {
  (cd "$1" &&
    find . \( -path ./var -o -path ./hello-doinst-ran \) -prune -o \
      -printf '%p %y %m %u %g %s %l\n' | LC_ALL=C sort &&
    find . \( -path ./var -o -path ./hello-doinst-ran \) -prune -o \
      -type f -exec md5sum {} + | LC_ALL=C sort &&
    for _d in var/lib/pkgtools/packages var/lib/pkgtools/scripts \
              var/lib/pkgtools/douninst.sh var/log/pkgtools/removed_packages \
              var/log/pkgtools/removed_scripts \
              var/log/pkgtools/removed_uninstall_scripts; do
      [ -d "$_d" ] && ls "$_d" | sed "s,^,$_d/,; s/-\(upgraded\|removed\)-[0-9:,-]*$//"
    done)
}
same_tree() { # same_tree ROOT1 ROOT2
  pt_tree "$1" > "$TMP/tree1"; pt_tree "$2" > "$TMP/tree2"
  diff "$TMP/tree1" "$TMP/tree2" > "$TMP/tree-diff" && return 0
  sed 's/^/    | /' "$TMP/tree-diff"; return 1
}
# a database entry without the size lines
entry() { grep -v '^\(UN\)\?COMPRESSED PACKAGE SIZE:' "$1/var/lib/pkgtools/packages/$2"; }
same_entry() { # same_entry ROOT1 ROOT2 PKGNAME
  entry "$1" "$3" > "$TMP/entry1"; entry "$2" "$3" > "$TMP/entry2"
  diff "$TMP/entry1" "$TMP/entry2" > "$TMP/entry-diff" && return 0
  sed 's/^/    | /' "$TMP/entry-diff"; return 1
}
same_file_list() { # same_file_list ROOT1 ROOT2 PKGNAME
  sed -n '/^FILE LIST:/,$p' "$1/var/lib/pkgtools/packages/$3" > "$TMP/list1"
  sed -n '/^FILE LIST:/,$p' "$2/var/lib/pkgtools/packages/$3" > "$TMP/list2"
  diff "$TMP/list1" "$TMP/list2" > "$TMP/list-diff" && return 0
  sed 's/^/    | /' "$TMP/list-diff"; return 1
}
same_file() { cmp -s "$1" "$2"; }
# the mode of hello-1's script in removed_uninstall_scripts in $PT and $SP
same_uninst_mode() {
  [ "$(stat -c %a "$PT"/var/log/pkgtools/removed_uninstall_scripts/hello-1-noarch-1*)" = \
    "$(stat -c %a "$SP"/var/log/pkgtools/removed_uninstall_scripts/hello-1-noarch-1*)" ]
}

if [ ! -x "$PKGTOOLS/installpkg" ] || [ ! -x "$PKGTOOLS/removepkg" ] ||
   [ ! -x "$PKGTOOLS/upgradepkg" ]; then
  begin "pkgtools"
  echo "    skipped, pkgtools not found in $PKGTOOLS"
else
  PT=$TMP/root-pt SP=$TMP/root-sp
  db=var/lib/pkgtools

  begin "pkgtools: spkg -i installs like installpkg"
  rm -rf "$PT" "$SP"; mkdir "$PT" "$SP"
  pt "$PT" installpkg "$PKGS/hello-1-noarch-1.tgz"
  check "installpkg exit status 0" rc_is 0
  ROOT=$SP; spkg -i "$PKGS/hello-1-noarch-1.tgz"
  check "spkg exit status 0" rc_is 0
  check "same files and database names" same_tree "$PT" "$SP"
  check "same file list in the entry" same_file_list "$PT" "$SP" hello-1-noarch-1
  check "same database entry" same_entry "$PT" "$SP" hello-1-noarch-1
  check "same doinst.sh stored" same_file "$PT/$db/scripts/hello-1-noarch-1" "$SP/$db/scripts/hello-1-noarch-1"
  check "same douninst.sh stored" same_file "$PT/$db/douninst.sh/hello-1-noarch-1" "$SP/$db/douninst.sh/hello-1-noarch-1"
  check "same douninst.sh mode" [ "$(stat -c %a "$PT/$db/douninst.sh/hello-1-noarch-1")" = \
    "$(stat -c %a "$SP/$db/douninst.sh/hello-1-noarch-1")" ]
  for l in packages scripts setup; do
    check "var/log/$l compatibility symlink like installpkg" \
      [ "$(readlink "$SP/var/log/$l")" = "$(readlink "$PT/var/log/$l")" ]
  done

  # Both start from the same installpkg install, then pkgtools or spkg
  # does the next step.
  pt_pair() { # pt_pair PKG : installpkg PKG into both roots
    rm -rf "$PT" "$SP"; mkdir "$PT" "$SP"
    pt "$PT" installpkg "$1"; pt "$SP" installpkg "$1"
  }

  begin "pkgtools: spkg -d removes a package installed by installpkg"
  pt_pair "$PKGS/hello-1-noarch-1.tgz"
  pt "$PT" removepkg hello
  check "removepkg exit status 0" rc_is 0
  ROOT=$SP; spkg -d hello
  check "spkg exit status 0" rc_is 0
  check "same result as removepkg" same_tree "$PT" "$SP"
  check "same removed entry" same_file "$PT"/var/log/pkgtools/removed_packages/hello-1-noarch-1* \
    "$SP"/var/log/pkgtools/removed_packages/hello-1-noarch-1*
  check "same mode of the removed uninstall script" same_uninst_mode

  begin "pkgtools: spkg -u upgrades a package installed by installpkg"
  pt_pair "$PKGS/hello-1-noarch-1.tgz"
  pt "$PT" upgradepkg "$PKGS/hello-2-noarch-1.tgz"
  check "upgradepkg exit status 0" rc_is 0
  ROOT=$SP; spkg -u "$PKGS/hello-2-noarch-1.tgz"
  check "spkg exit status 0" rc_is 0
  check "same result as upgradepkg" same_tree "$PT" "$SP"
  check "same scripts ran" [ "$(cat "$PT/script-log")" = "$(cat "$SP/script-log")" ]
  check "same mode of the removed uninstall script" same_uninst_mode

  # Both start from the same spkg install, then pkgtools or spkg does the
  # next step.
  sp_pair() { # sp_pair PKG : spkg -i PKG into both roots
    rm -rf "$PT" "$SP"; mkdir "$PT" "$SP"
    ROOT=$PT; spkg -i "$1"; ROOT=$SP; spkg -i "$1"
  }

  begin "pkgtools: removepkg removes a package installed by spkg"
  sp_pair "$PKGS/hello-1-noarch-1.tgz"
  pt "$PT" removepkg hello
  check "removepkg exit status 0" rc_is 0
  ROOT=$SP; spkg -d hello
  check "same result as spkg -d" same_tree "$PT" "$SP"
  check "douninst.sh ran" grep -q "^douninst 1" "$PT/script-log"

  begin "pkgtools: upgradepkg upgrades a package installed by spkg"
  sp_pair "$PKGS/hello-1-noarch-1.tgz"
  pt "$PT" upgradepkg "$PKGS/hello-2-noarch-1.tgz"
  check "upgradepkg exit status 0" rc_is 0
  ROOT=$SP; spkg -u "$PKGS/hello-2-noarch-1.tgz"
  check "same result as spkg -u" same_tree "$PT" "$SP"
  check "same scripts ran" [ "$(cat "$PT/script-log")" = "$(cat "$SP/script-log")" ]

  begin "pkgtools: spkg reads a database entry written by installpkg"
  pt_pair "$PKGS/hello-1-noarch-1.tgz"
  ROOT=$PT; spkg -l
  check "list exit status 0" rc_is 0
  check "listed" output_has "^hello-1-noarch-1$"
fi

# ---------------------------------------------------------------------

echo
if [ $failures -ne 0 ]; then
  echo "$failures check(s) failed"
  exit 1
fi
echo "all checks passed"
exit 0
