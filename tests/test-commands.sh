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

# snapshot without install/ (see the install/doinst.sh rollback xfails)
snapshot_no_install() {
  snapshot | grep -v -e '^\./install' -e '^\. d' -e ' \./install'
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
         var/log/pkgtools/removed_packages var/log/pkgtools/removed_scripts; do
  check "$d exists" is_dir "$d"
done

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

begin "install: --dry-run changes nothing"
new_root dry-install
spkg -i "$PKGS/hello-1-noarch-1.tgz"   # creates the database dirs
spkg -d hello
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
xfail "rolled back: install/doinst.sh not left behind" absent install

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
check "no copy left in var/tmp/spkg" absent var/tmp/spkg/hello-1-noarch-1

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
before=$(snapshot_no_install)
cp "$PKGS/trunc-1.0-noarch-1.tgz" "$PKGS/hello-3-noarch-1.tgz"
spkg -u "$PKGS/hello-3-noarch-1.tgz"
check "exit status 2" rc_is 2
check "old version still in the database" db_has hello-1-noarch-1
check "tree unchanged (apart from install/)" [ "$before" = "$(snapshot_no_install)" ]
xfail "rolled back: install/doinst.sh not left behind" absent install

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
check "no copy left in var/tmp/spkg" absent var/tmp/spkg/hello-1-noarch-1
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
st=$TMP/stage-hello-3
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
before=$(snapshot_no_install)
cp "$PKGS/brokenuninst-1-noarch-1.tgz" "$PKGS/hello-4-noarch-1.tgz"
spkg -u "$PKGS/hello-4-noarch-1.tgz"
check "exit status 2" rc_is 2
check "old script not run" log_is "doinst 1"
check "old script still stored" douninst_stored hello-1-noarch-1
check "tree unchanged (apart from install/)" [ "$before" = "$(snapshot_no_install)" ]

begin "upgrade: --dry-run doesn't run douninst.sh"
new_root upgrade-douninst-dry
spkg -i "$PKGS/hello-1-noarch-1.tgz"
before=$(snapshot)
spkg -u -n "$PKGS/hello-2-noarch-1.tgz"
check "exit status 0" rc_is 0
check "douninst.sh not run" log_is "doinst 1"
check "tree unchanged" [ "$before" = "$(snapshot)" ]

# ---------------------------------------------------------------------

echo
if [ $failures -ne 0 ]; then
  echo "$failures check(s) failed"
  exit 1
fi
echo "all checks passed"
exit 0
