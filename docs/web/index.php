<?php require "common.php"; ?>
<?php head("spkg - main page"); ?>

  <p>Welcome to the official website of spkg, the unofficial <a
  href="http://slackware.com">Slackware Linux</a> package manager. spkg
  is implemented in C and optimized for speed. The latest version
  is: <b>spkg-@VER@</b>.</p>

  <p><b>Spkg is now feature complete and under heavy testing by yours
  truly.</b></p>

 <h2>Features</h2>

  <ul>
    <li>Extreme symplicity. Just like pkgtools.</li>
    <li>Fast installation (approx. 5% faster than <b>tar xzf</b>)</li>
    <li>Fast uninstallation (faster than <b>rm -rf</b>)</li>
    <li>Fast upgrade too, I guess... :-)</li>
    <li>Command to list information about installed packages.</li>
    <li>Robust implementation. (nearly all error conditions are checked)</li>
    <li>Rollback functionality. (no file left behind policy ;-))</li>
    <li>Full compatibility with legacy Slackware package database.</li>
    <li>Everything is libified. (see <a href="docs.php">docs</a>) You can 
    implement new commands easily.</li>
    <li>Easy access to the package database thanks to libification.</li>
  </ul>

  <p><b>Planned features are (comming after version 1.0.0):</b></p>
  <ul>
    <li>Python bindings.</li>
    <li>PyGtk based GUI.</li>
  </ul>

 <h2>News</h2>

<h3>Spkg beta released (2006-07-17)</h3>
     <ul>
<li>Spkg is feature complete!</li>
<li>Less screwed website design.</li>
<li>Upgrade command implemented. It is not tested very much.
    I've just tested it on the samba package from the slackware
    current and it works same as upgradepkg from pkgtools.
    It's based on the cmd_install() code though, so it should
    work quit well. Time will tell. </li>
<li>Install command refactoring: cmd_install() was split into
    smaller and easilly manageable functions.</li>
<li>Memory allocation audit. (confirmed by valgrind and glib
    memory allocation profiler)</li>
<li>Root path sanitization. (fixes double slashes when --root /)</li>
<li>Improved and more consistent output from commands.</li>
<li>Added more warnings where necessary. (permission diferences
    between installed and existing directories, files changed
    after installation, etc.)</li>
<li>--dry-run should be now really DRY. :)</li>
<li>Perform sanity checks on paths extracted from the doinst.sh
    script.</li>
<li>Added dep packages download script, for those who want
    to build static verion of the spkg and don't want to search
    whole day for popt-1.10.2 sources on the internet. ;-)</li>
<li>Improved package name guessing algorithm.</li>
     </ul>

<h3>What's comming</h3>
     <ul>
<li>There will be refactoring of the transaction code. (it's
    not broken, but it's not quit readable too)</li>
<li>Then, I will write automated testsuite and create some really    
    torturous packages to test as much code paths in spkg as
    possible. And of course I will be running spkg in my normal
    day to day use.</li>
<li>Finally I would like to update documentation and make some
    benchamrks on the brand new --upgrade command.</li>
<li>Please, keep an eye on 
    <a href="http://spkg.megous.com/dl/patches/beta/">http://spkg.megous.com/dl/patches/beta/</a>
    for patches that will fix bugs found in this beta release.
    I will be adding patches there as soon as each bug is fixed.
    To date there are no known bugs in spkg.</li>
     </ul>

 <p>See <a href="history.php">older news</a>...</p>

<?php foot(); ?>
