<?php require "common.php"; ?>

 <h2>History</h2>

   <h3>2006-07-17</h3>
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

   <h3>2006-07-14</h3>
    <p>I started publishing patches to the alpha1 version of spkg
    in the <a href="dl/patches">patches</a> directory.</p>
   <h3>2006-07-10</h3>
    <p>Firtst alpha version released! This version implements install,
    remove and list commands. See manpage for more information.
    
    Other new features include fully functional command line interface
    with corresponding manpage and implementation of safe break points.
    
    You can safely break any command using some reasonable signal like
    SIGINT and all changes made so far will be automatically rolled back.
    
    There are new command verbosity selection options too.

    I've put spkg on diet and completely dropped sqlite and filedb
    database code. This results in fewer dependencies and little to
    no performance impact. Filedb code was replaced with JudySL
    arrays. See <a href="http://judy.sourceforge.net">this</a> page
    for more info.
    
    Enjoy it!</p>
   <h3>2005-07-17</h3>
    <p>Install command is nearly completed. spkg installs 60 packages
    (total size 18MB) under 3 seconds. Installpkg from pkgtools needs 90
    seconds to install the same set of packages. Whooooa! Maybe I should
    reabbreviate spkg for speeeeedy package manager. :] Keep tuned.</p>
   <h3>2005-06-29</h3>
    <p>Pkgdb completed.</p>
   <h3>2005-06-27</h3>
    <p>Filedb library received new features: fast distributed
    checksumming and per file arbitrary data storing. It is possible
    to open multiple file databases at once. Implemented new, much
    better error handling everywhere. <a href="dl/TODO">TODO</a>
    file is now automatically updated on the web.</p>
   <h3>2005-06-21</h3>
    <p>I've implemented <a href="dl/spkg-docs/pyspkg.html">Python
    bindings</a>. Now I'm finishing pkgdb library.</p>
   <h3>2005-06-14</h3>
    <p>SQL database wrapper API completed. Untgz documented.</p>
   <h3>2005-06-12</h3>
    <p>Another milestone achieved. Pkgdb is fast. Database operations on 
    averange package can be done in 2ms on my 1GHz Athlon. Full 
    synchronization with legacy database on my system can be done in 1 second.
    (522 packages)</p>
   <h3>2005-06-10</h3>
    <p>Website created. Development code released.</p>
  </dl>

<?php foot(); ?>
